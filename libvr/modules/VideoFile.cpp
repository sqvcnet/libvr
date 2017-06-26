//
//  VideoFile.cpp
//  vrplayer
//
//  Created by 单强 on 16/5/20.
//  Copyright © 2016年 单强. All rights reserved.
//
#include "main.h"
#include "Util.h"
#include "VideoFile.hpp"
#include "FileUtil.hpp"

namespace geeek {
    
VideoFile::VideoFile()
: _avFormatCtx(nullptr)
, _videoStreamIndex(-1)
, _audioStreamIndex(-1)
, _audioCodec(nullptr)
, _videoCodec(nullptr)
, _openRet(0)
{
}

void VideoFile::close() {
    if (_avFormatCtx) {
//        avformat_free_context(_avFormatCtx); will be free in avformat_close_input
        avformat_close_input(&_avFormatCtx);
        _avFormatCtx = nullptr;
    }
    _videoStreamIndex = -1;
    _audioStreamIndex = -1;
}

VideoFile::~VideoFile() {
    close();
}

bool VideoFile::hasAudio() const {
    return _audioStreamIndex >= 0;
}

bool VideoFile::hasVideo() const {
    return _videoStreamIndex >= 0;
}

bool VideoFile::isAudio(int streamIndex) const {
    return _audioStreamIndex == streamIndex;
}

bool VideoFile::isVideo(int streamIndex) const {
    return _videoStreamIndex == streamIndex;
}

double VideoFile::getProgress(int64_t videoDts, int64_t audioDts) {
    if (!_avFormatCtx) {
        return -1.0;
    }
    //以视频为准，因为视频向音频同步，滞后音频一帧
    AVStream *stream = nullptr;
    double d1 = 0.0, d2 = 0.0;
    if (hasVideo()) {
        if (videoDts == AV_NOPTS_VALUE) {
            return -1.0;
        }
        stream = _avFormatCtx->streams[_videoStreamIndex];
        d1 = 1.0 * (videoDts - stream->first_dts) / stream->duration;
    }
    if (hasAudio()) {
        if (audioDts == AV_NOPTS_VALUE) {
            return -1.0;
        }
        stream = _avFormatCtx->streams[_audioStreamIndex];
        d2 =  1.0 * (audioDts - stream->first_dts) / stream->duration;
    }
    if (hasAudio() && hasVideo()) {
        return min(d1, d2);
    } else if (hasVideo()) {
        return d1;
    } else if (hasAudio()) {
        return d2;
    }
    return -1.0;
}

double VideoFile::getTotalTime() {
    if (!_avFormatCtx) {
        return 0.0;
    }
    return _avFormatCtx->duration / AV_TIME_BASE;
}

int VideoFile::seekAudio(double percent) {
    if (!_avFormatCtx) {
        return -1;
    }
    AVStream *stream = _avFormatCtx->streams[_audioStreamIndex];
    int ret = av_seek_frame(_avFormatCtx, _audioStreamIndex, stream->first_dts + percent * stream->duration, AVSEEK_FLAG_BACKWARD);
    LOGD("VideoFile::seek audio ret: %d", ret);
    return ret;
}

int VideoFile::seekVideo(double percent) {
    if (!_avFormatCtx) {
        return -1;
    }
    AVStream *stream = _avFormatCtx->streams[_videoStreamIndex];
    int ret = av_seek_frame(_avFormatCtx, _videoStreamIndex, stream->first_dts + percent * stream->duration, AVSEEK_FLAG_BACKWARD);
    LOGD("VideoFile::seek video ret: %d", ret);
    return ret;
}

int VideoFile::readFrame(AVPacket *packet) {
    if(av_read_frame(_avFormatCtx, packet) < 0) {
        if(_avFormatCtx->pb->error == 0) {
            //eof
            return -1;
        } else {
            //error
            return -2;
        }
    }
    return 0;
}

int VideoFile::openCb(void *p) {
    VideoFile *self = reinterpret_cast<VideoFile*>(p);
    return self->_openRet;
}

void VideoFile::abortOpen() {
    _openRet = -1;
}
    
void VideoFile::open(const string & srcPath) {
    //assure the open resource be released
    close();
    
    string path = FileUtil::getFullPath(srcPath);
    LOGD("VideoFile::open %s", path.c_str());

    _openRet = 0;
    
    _avFormatCtx = avformat_alloc_context();
//    _avFormatCtx->flags |= AVFMT_FLAG_NONBLOCK;
    _avFormatCtx->interrupt_callback.callback = VideoFile::openCb;
    _avFormatCtx->interrupt_callback.opaque = this;
    
    static bool initOnce = false;
    if (!initOnce) {
        av_register_all();
        avcodec_register_all();
        avformat_network_init();
        initOnce = true;
    }
    
    int ret = 0;
    char str[1024] = {'\0'};
    if((ret = avformat_open_input(&_avFormatCtx, path.c_str(), nullptr, nullptr)) != 0) {
        av_strerror(ret, str, 1024);
        throw runtime_error(string("avformat_open_input error: ") + str);
        return;
    }
    
    // Retrieve stream information
    if((ret = avformat_find_stream_info(_avFormatCtx, NULL)) < 0) {
        av_strerror(ret, str, 1024);
        throw runtime_error(string("avformat_find_stream_info error: ") + str);
        return;
    }
    
    // Dump information about file onto standard error
    av_dump_format(_avFormatCtx, 0, path.c_str(), 0);
    
    for(int i = 0; i < _avFormatCtx->nb_streams; i++) {
        if(_avFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO &&
           _videoStreamIndex < 0) {
            _videoStreamIndex = i;
            _videoCodecId = _avFormatCtx->streams[i]->codecpar->codec_id;
            _videoCodec = _avFormatCtx->streams[i]->codec;
        }
        if(_avFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO &&
           _audioStreamIndex < 0) {
            _audioStreamIndex = i;
            _audioCodecId = _avFormatCtx->streams[i]->codecpar->codec_id;
            _audioCodec = _avFormatCtx->streams[i]->codec;
        }
    }

    return;
}

AVCodecContext* VideoFile::getAudioCodec() const {
    return _audioCodec;
}

AVCodecContext* VideoFile::getVideoCodec() const {
    return _videoCodec;
}

enum AVCodecID VideoFile::getAudioCodecId() const {
    return _audioCodecId;
}

enum AVCodecID VideoFile::getVideoCodecId() const {
    return _videoCodecId;
}

double VideoFile::getExternalClock() {
    return av_gettime() / 1000000.0;
}

}
