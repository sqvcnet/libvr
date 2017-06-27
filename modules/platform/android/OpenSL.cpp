//
//  OpenSL.cpp
//  vrplayer
//
//  Created by 单强 on 16/7/27.
//  Copyright © 2016年 Facebook. All rights reserved.
//

#include "OpenSL.hpp"

OpenSL::OpenSL()
: _engineObject(nullptr)
, _engineEngine(nullptr)
, _outputMixObject(nullptr)
, _outputMixPresetReverb(nullptr)
, _bqPlayerObject(nullptr)
, _bqPlayerPlay(nullptr)
, _bqPlayerBufferQueue(nullptr)
, _bqPlayerEffectSend(nullptr)
, _bqPlayerVolume(nullptr)
, _delegate(nullptr)
, _isExitThread(true)
, _isClosed(true)
, _lastCount(0)
{
    
}

OpenSL::~OpenSL() {
    uninit();
}

void OpenSL::uninit() {
    close();
    
    // destroy output mix object, and invalidate all associated interfaces
    if (_outputMixObject != nullptr) {
        (*_outputMixObject)->Destroy(_outputMixObject);
        _outputMixObject = nullptr;
        _outputMixPresetReverb = nullptr;
    }
    
    // destroy engine object, and invalidate all associated interfaces
    if (_engineObject != NULL) {
        (*_engineObject)->Destroy(_engineObject);
        _engineObject = nullptr;
        _engineEngine = nullptr;
    }
}

SLresult OpenSL::init(AudioOutputDelegate *delegate) {
    _delegate = delegate;
    
    uninit();
    LOGV("OpenSL init start");
    SLresult result = slCreateEngine(&_engineObject, 0, NULL, 0, NULL, NULL);
    if (result != SL_RESULT_SUCCESS) return result;
    result = (*_engineObject)->Realize(_engineObject, SL_BOOLEAN_FALSE);
    if (result != SL_RESULT_SUCCESS) return result;
    
    result = (*_engineObject)->GetInterface(_engineObject,
                                            SL_IID_ENGINE, &_engineEngine);
    if (result != SL_RESULT_SUCCESS) return result;
    
    const SLInterfaceID ids[1] = {SL_IID_PRESETREVERB};
    const SLboolean req[1] = {SL_BOOLEAN_FALSE};
    result = (*_engineEngine)->CreateOutputMix(_engineEngine,
                                               &_outputMixObject, 1, ids, req);
    if (result != SL_RESULT_SUCCESS) return result;
    result = (*_outputMixObject)->Realize(_outputMixObject, SL_BOOLEAN_FALSE);
    if (result != SL_RESULT_SUCCESS) return result;
    
    result = (*_outputMixObject)->GetInterface(_outputMixObject, SL_IID_PRESETREVERB,
                                               &_outputMixPresetReverb);
    if (SL_RESULT_SUCCESS == result) {
        result = (*_outputMixPresetReverb)->SetPreset(_outputMixPresetReverb, SL_REVERBPRESET_MEDIUMROOM);
        LOGV("OpenSL init SL_IID_PRESETREVERB success");
        (void)result;
    }
    
    LOGV("OpenSL init success");
    return SL_RESULT_SUCCESS;
}

bool OpenSL::writeBuffer(const char *data, int size) {
    SLresult slRet = (*_bqPlayerBufferQueue)->Enqueue(_bqPlayerBufferQueue, data, size);
    if (slRet == SL_RESULT_SUCCESS) {
    } else {
        LOGV("Enqueue failed: %d\n", (int)slRet);
        return false;
    }
    
    return true;
}

void OpenSL::playCallback(SLBufferQueueItf caller, void *context) {
    //    LOGV("Audio playCallback");
    OpenSL *self = (OpenSL*)context;
    if (self->_isExitThread) {
        self->_isClosed = true;
        return;
    }
    
    SLBufferQueueState st;
    SLresult res = (*(self->_bqPlayerBufferQueue))->GetState(self->_bqPlayerBufferQueue, &st);
    if (res != SL_RESULT_SUCCESS) {
        LOGV("GetState failed!!!");
        return;
    }
    //    LOGV("st.index=%d, st.count=%d", st.playIndex, st.count);
    if (st.count >= self->OPENSL_BUFFERS) {
        LOGV("Buffers full!!!");
        return;
    }

    self->_delegate->consumeFrames(self->_lastCount - st.count);
    char *data = nullptr;
    size_t size = 0;
    while (st.count < self->OPENSL_BUFFERS) {
        if (self->_delegate->getFrame(&data, &size)) {
            self->writeBuffer(data, size);
        } else {
            break;
        }
        st.count++;
    }
    self->_lastCount = st.count;
    
    if (st.count == 0) {
        self->writeBuffer(self->SILENCE4PACK, self->SILENCE_SIZE);
        LOGD("OpenSL::playCallback write silence");
        self->_lastCount = 1;
    }
}

SLresult OpenSL::open(SLuint32 channels, SLuint32 sampleRate) {
    LOGD("OpenSL open channels: %u, sampleRate: %u", channels, sampleRate);
    if (!_isExitThread) {
        LOGD("OpenSL::open: already opened");
        return SL_RESULT_SUCCESS;
    }
    
    sampleRate *= 1000;
    SLuint32 speakers;
    if(channels > 1)
        speakers = SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT;
    else
        speakers = SL_SPEAKER_FRONT_CENTER;
    
    SLDataLocator_BufferQueue loc_bufq = {SL_DATALOCATOR_BUFFERQUEUE, OPENSL_BUFFERS};
    SLDataFormat_PCM format_pcm = {SL_DATAFORMAT_PCM,channels,sampleRate,
        SL_PCMSAMPLEFORMAT_FIXED_16, SL_PCMSAMPLEFORMAT_FIXED_16,
        speakers, SL_BYTEORDER_LITTLEENDIAN};
    SLDataSource audioSrc = {&loc_bufq, &format_pcm};
    
    //    LOGV("Audio open _outputMixObject: %x", _outputMixObject);
    SLDataLocator_OutputMix loc_outmix = {SL_DATALOCATOR_OUTPUTMIX,
        _outputMixObject};
    SLDataSink audioSnk = {&loc_outmix, nullptr};
    
    const SLInterfaceID ids[] = {SL_IID_BUFFERQUEUE, SL_IID_EFFECTSEND, SL_IID_VOLUME, SL_IID_PLAY};
    const SLboolean req[] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE};
    
    SLresult result = (*_engineEngine)->CreateAudioPlayer(_engineEngine, &_bqPlayerObject, &audioSrc, &audioSnk, sizeof(ids) / sizeof(*ids), ids, req);
    LOGD("OpenSL open CreateAudioPlayer result: %u", result);
    if (result != SL_RESULT_SUCCESS) return result;
    LOGD("OpenSL open CreateAudioPlayer success");
    result = (*_bqPlayerObject)->Realize(_bqPlayerObject,
                                         SL_BOOLEAN_FALSE);
    if (result != SL_RESULT_SUCCESS) return result;
    LOGD("OpenSL open Realize _bqPlayerObject success");
    result = (*_bqPlayerObject)->GetInterface(_bqPlayerObject, SL_IID_PLAY, &_bqPlayerPlay);
    if (result != SL_RESULT_SUCCESS) return result;
    LOGD("OpenSL open SL_IID_PLAY success");
    result = (*_bqPlayerObject)->GetInterface(_bqPlayerObject, SL_IID_VOLUME, &_bqPlayerVolume);
    if (result != SL_RESULT_SUCCESS) return result;
    LOGD("OpenSL open SL_IID_VOLUME success");
    result = (*_bqPlayerObject)->GetInterface(_bqPlayerObject, SL_IID_BUFFERQUEUE, &_bqPlayerBufferQueue);
    if (result != SL_RESULT_SUCCESS) return result;
    LOGD("OpenSL open SL_IID_BUFFERQUEUE success");
    
    result = (*_bqPlayerObject)->GetInterface(_bqPlayerObject, SL_IID_EFFECTSEND, &_bqPlayerEffectSend);
    if (result != SL_RESULT_SUCCESS) return result;
    LOGD("OpenSL open SL_IID_EFFECTSEND success");
    
//    result = (*_bqPlayerEffectSend)->EnableEffectSend(_bqPlayerEffectSend,
//                                                      _outputMixPresetReverb, (SLboolean) SL_BOOLEAN_TRUE, (SLmillibel) 0);
//    if (SL_RESULT_SUCCESS == result) {
//        LOGD("OpenSL open EnableEffectSend success");
//    }
    
    result = (*_bqPlayerBufferQueue)->RegisterCallback(_bqPlayerBufferQueue, playCallback, this);
    if (result != SL_RESULT_SUCCESS) return result;
    LOGD("OpenSL open RegisterCallback success");
    result = (*_bqPlayerPlay)->SetPlayState(_bqPlayerPlay, SL_PLAYSTATE_PLAYING);
    if (result != SL_RESULT_SUCCESS) return result;
    
    _isExitThread = false;
    _isClosed = false;
    _lastCount = 0;

    //active buffer queue
    writeBuffer(SILENCE4PACK, SILENCE_SIZE);

    LOGD("OpenSL open success");
    return SL_RESULT_SUCCESS;
}

bool OpenSL::isOpened() {
    return !_isClosed;
}

void OpenSL::close() {
    if (!_isExitThread) {
        _isExitThread = true;
        while (!_isClosed) {
            this_thread::sleep_for(chrono::milliseconds(50));
        }
    }
    // destroy buffer queue audio player object, and invalidate all associated interfaces
    if (_bqPlayerObject != nullptr) {
        (*_bqPlayerObject)->Destroy(_bqPlayerObject);
        _bqPlayerObject = nullptr;
        _bqPlayerPlay = nullptr;
        _bqPlayerBufferQueue = nullptr;
        _bqPlayerEffectSend = nullptr;
        _bqPlayerVolume = nullptr;
    }
}
