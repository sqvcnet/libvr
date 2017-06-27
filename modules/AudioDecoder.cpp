//
//  AudioDecoder.cpp
//  vrplayer
//
//  Created by 单强 on 2017/3/12.
//  Copyright © 2017年 Facebook. All rights reserved.
//

#include "AudioDecoder.hpp"
#include "PacketReader.hpp"

namespace geeek {
    
AudioDecoder::AudioDecoder(PacketReader *packetReader)
: _packetReader(packetReader)
, _audioFrame(nullptr)
, _audioCtx(nullptr)
, _swrCtx(nullptr)
, _outSampleRate(0)
, _curDts(AV_NOPTS_VALUE)
, _dstAudioData(nullptr)
, _srcNbSamples(0)
, _dstNbSamples(0)
, _dstSampleFmt(AV_SAMPLE_FMT_S16)
, _audioNbSamples(0)
, _outChannels(2) {
    _audioFrame = av_frame_alloc();
}

AudioDecoder::~AudioDecoder() {
    close();
    if (_audioFrame) {
        av_frame_free(&_audioFrame);
        _audioFrame = nullptr;
    }
}

void AudioDecoder::freeAudioBuffer() {
    if (_dstAudioData)
        av_freep(&_dstAudioData[0]);
    av_freep(&_dstAudioData);
    _srcNbSamples = 0;
}

bool AudioDecoder::allocAudioBuffer(int nbSamples) {
    freeAudioBuffer();
    
    _srcNbSamples = nbSamples;
    _dstNbSamples = _srcNbSamples * _outSampleRate / _audioCtx->sample_rate + 256;
    
    int dstLineSize = 0;
    int dstDataSize = av_samples_alloc_array_and_samples(&_dstAudioData, &dstLineSize, _outChannels, _dstNbSamples, _dstSampleFmt, 0);
    if (dstDataSize < 0) {
        LOGE("Could not allocate destination samples: %d\n", _dstNbSamples);
        return false;
    }
    
    return true;
}

PtsFrame AudioDecoder::getAudio() {
    PtsFrame audio;
    int gotFrame = 0;
    int size = -1;
    AVPacket *packet = nullptr;
    
    _packetReader->consumeAudioPacket(&packet);
    if (nullptr == packet) {
        LOGD("getAudio: null packet");
        return audio;
    }
    size = avcodec_decode_audio4(_audioCtx, _audioFrame, &gotFrame, packet);
    int64_t curDts = packet->dts;
    av_free_packet(packet);
    delete packet;
    
    if (size < 0) {
        LOGE("avcodec_decode_audio4 error!");
        return audio;
    }
    
    if (!gotFrame) {
        return audio;
    }
    
    if (_audioFrame->nb_samples != _srcNbSamples) {
        if (!allocAudioBuffer(_audioFrame->nb_samples)) {
            return audio;
        }
    }
    
    int len = swr_convert(_swrCtx, _dstAudioData, _dstNbSamples, (const uint8_t **)_audioFrame->data, _audioFrame->nb_samples);
    if (len < 0) {
        LOGE("swr_convert error!");
        return audio;
    }
    
    _curDts = curDts;
    
    audio.first = _audioNbSamples;
    _audioNbSamples += len;
    audio.second = string(reinterpret_cast<char *>(_dstAudioData[0]), len * _outChannels * sizeof(int16_t));
    return audio;
}

int64_t AudioDecoder::getCurDts() {
    return _curDts;
}

int AudioDecoder::getOutputChannels() {
    return _outChannels;
}

int AudioDecoder::getSampleRate() {
    if (_audioCtx) {
        return _audioCtx->sample_rate;
    }
    return 0;
}

int AudioDecoder::getOutputSampleRate() {
    return _outSampleRate;
}

int AudioDecoder::open(AVCodecContext *audioCodec, enum AVCodecID codecId) {
    _curDts = AV_NOPTS_VALUE;
    _audioNbSamples = 0;
    
    _audioCtx = nullptr;
    AVCodec *codec = avcodec_find_decoder(codecId);
    
    int ret = 0;
    if(!codec) {
        LOGE("%s", "audio codec is null!\n");
        ret = -1;
        goto fail;
    }
    LOGD("audio codec name: %s\n", codec->name);
    _audioCtx = avcodec_alloc_context3(codec);
    if(avcodec_copy_context(_audioCtx, audioCodec) != 0) {
        LOGE("%s", "Couldn't copy audio codec context");
        ret = -2;
        goto fail;
    }
    
    if((ret = avcodec_open2(_audioCtx, NULL, NULL)) < 0) {
        char ac[1024];
        av_strerror(ret, ac, 1024);
        LOGE("%s, error: %d, str: %s\n", "Unsupported audio codec!", ret, ac);
        ret = -3;
        goto fail;
    }
    
    _swrCtx = swr_alloc();
    if (_audioCtx) {
        _outSampleRate = _audioCtx->sample_rate;
        if (_audioCtx->sample_rate < 4000) {
            _outSampleRate = 4000;
        }
        if (_audioCtx->sample_rate > 48000) {
            _outSampleRate = 48000;
        }
        
        int64_t in_channel_layout = (_audioCtx->channel_layout &&
                         _audioCtx->channels ==
                         av_get_channel_layout_nb_channels(_audioCtx->channel_layout)) ?
        _audioCtx->channel_layout :
        av_get_default_channel_layout(_audioCtx->channels);
        
        av_opt_set_int(_swrCtx, "in_channel_layout", in_channel_layout, 0);
        av_opt_set_int(_swrCtx, "out_channel_layout", AV_CH_LAYOUT_STEREO, 0);
        av_opt_set_int(_swrCtx, "in_sample_rate", _audioCtx->sample_rate, 0);
        av_opt_set_int(_swrCtx, "out_sample_rate", _outSampleRate, 0);
        av_opt_set_sample_fmt(_swrCtx, "in_sample_fmt", _audioCtx->sample_fmt, 0);
        av_opt_set_sample_fmt(_swrCtx, "out_sample_fmt", _dstSampleFmt, 0);
        /* initialize the resampling context */
        if (swr_init(_swrCtx) < 0) {
            LOGE("Failed to initialize the resampling context\n");
            ret = -4;
            goto fail;
        }
    }
    
    return ret;
    
fail:
    if (_audioCtx) {
        avcodec_free_context(&_audioCtx);
    }
    return ret;
}

void AudioDecoder::close() {
    if (_audioCtx) {
        avcodec_flush_buffers(_audioCtx);
        avcodec_free_context(&_audioCtx);
        _audioCtx = nullptr;
    }
    
    freeAudioBuffer();
    
    if (_swrCtx) {
        swr_free(&_swrCtx);
        _swrCtx = nullptr;
    }
    
    _curDts = AV_NOPTS_VALUE;
}

}
