//
//  Player.cpp
//  vrplayer
//
//  Created by 单强 on 16/6/14.
//  Copyright © 2016年 Facebook. All rights reserved.
//

#include "Player.hpp"
#include "main.h"
#include "Image.hpp"
#include "FileUtil.hpp"

namespace geeek {
    
Player::Player()
    :_audioCurNbSamples(AV_NOPTS_VALUE)
    ,_videoCurNbFrames(AV_NOPTS_VALUE)
    ,_srcPixFmt(AV_PIX_FMT_NONE)
    ,_dstPixFmt(AV_PIX_FMT_NONE)
    ,_isOpened(false)
    ,_openThread(nullptr)
    ,_srcFrameWidth(-1)
    ,_srcFrameHeight(-1)
    ,_dstFrameWidth(0)
    ,_dstFrameHeight(0)
    ,_lastError(Error::NO_ERROR)
    ,_openCallback(nullptr)
{
    _startPlayTime = av_gettime()/1000000.0;
    _packetReader = new PacketReader(this, &_videoFile, &_audio, &_video);
    _videoDecoder = new VideoDecoder(this, _packetReader);
    _audioDecoder = new AudioDecoder(_packetReader);
}

Player::~Player() {
    close();
    if (_packetReader) {
        delete _packetReader;
        _packetReader = nullptr;
    }
    if (_videoDecoder) {
        delete _videoDecoder;
        _videoDecoder = nullptr;
    }
    if (_audioDecoder) {
        delete _audioDecoder;
        _audioDecoder = nullptr;
    }
}

Renderer *Player::getRenderer() {
    return _video.getRenderer();
}

void Player::close() {
    if (_openThread) {
        _videoFile.abortOpen();
        _openThread->join();
        delete _openThread;
        _openThread = nullptr;
    }
    if (!_isOpened) {
        return;
    }
    pause();
    
    _audio.close();
    _video.close();
    
    _packetReader->stop();
    
    _audioDecoder->close();
    _videoDecoder->close();

    _videoFile.close();
    
    _isOpened = false;
    _lastError = Error::NO_ERROR;
}
    
void Player::setCodec(int codec) {
    if (codec != _videoDecoder->getCodec()) {
        _videoDecoder->setCodec(codec);
    }
}
    
void Player::pause() {
    LOGD("Player:pause start: %lld", av_gettime());
    _audio.pause();
    _video.pause();
    LOGD("Player:pause end: %lld", av_gettime());
    LOGD("Player has been paused");
}

void Player::play() {
    LOGD("Player::play");
    if (!_isOpened) {
        throw logic_error("play but not opened");
        return;
    }
    _audio.start();
    _video.start();
    if (_videoFile.hasVideo()) {
        _startPlayTime = av_gettime()/1000000.0 - _videoCurNbFrames * 1.0 / _videoDecoder->getFps();
        return;
    }
    if (_videoFile.hasAudio()) {
        _startPlayTime = av_gettime()/1000000.0 - _audioCurNbSamples * 1.0 / _audioDecoder->getOutputSampleRate();
        return;
    }
    throw runtime_error("neither audio nor video can be found");
}
    
void Player::start() {
    _audioCurNbSamples = 0;
    _videoCurNbFrames = 0;
    _packetReader->start();
    _startPlayTime = av_gettime()/1000000.0;
}
    
void Player::seek(double percent) {
    if (!_isOpened) {
        throw std::logic_error("seek but not opened");
        return;
    }
    LOGD("Player::seek start %f, progress: %f", percent, getCacheProgress());

    //只有open的情况下，Renderer::render才会走到loadTexture，否则_video.close会死等，audio同理

    _audio.stop();
    _video.stop();
    _packetReader->stop();

    _packetReader->seek(percent);

    start();
    _audio.start();
    _video.start();
    
    LOGD("Player::seek end %f, progress: %f", percent, getCacheProgress());
}
    
Player::Error Player::getLastError() {
    Player::Error err = _lastError;
    _lastError = Error::NO_ERROR;
    return err;
}

void Player::setLastError(Error err) {
    _lastError = err;
}
    
double Player::getProgress() {
    if (_packetReader->isEnd()) {
        pause();
        return 1.0;
    }
    return _videoFile.getProgress(_videoDecoder->getCurDts(), _audioDecoder->getCurDts());
}
    
double Player::getCacheProgress() {
    double progress = _packetReader->getCacheProgress();
//    LOGD("Player::getProgress: %f", progress);
    return progress;
}

float Player::getTotalTime() {
    double totalTime = _videoFile.getTotalTime();
//    LOGD("Player::getTotalTime: %f", totalTime);
    return totalTime;
}

double Player::computeAudioClock(int64_t nbSamples) {
    if (_videoFile.hasAudio()) {
//        int64_t nbSamples = _audio.getCurPts();
        int outRate = _audioDecoder->getOutputSampleRate();
        if (nbSamples == AV_NOPTS_VALUE) {
            //video 最快可以提前1帧播放
            double oneFrameTime = 1.0 / _videoDecoder->getFps();
            return 1.0 * _audioCurNbSamples / outRate + oneFrameTime;
        } else {
            _audioCurNbSamples = nbSamples;
        }
//        LOGD("nbSamples: %lld", nbSamples);
        return 1.0 * nbSamples / outRate;
    }
    return av_gettime()/1000000.0 - _startPlayTime;
}

double Player::computeVideoClock(int64_t nbFrames) {
    if (_videoFile.hasVideo()) {
        if (nbFrames == AV_NOPTS_VALUE) {
            //audio 最快可以比 video 快2帧，超过2帧播放静音来等
            nbFrames = _videoCurNbFrames + 2;
        } else {
            _videoCurNbFrames = nbFrames;
        }
    //    LOGD("nbFrames: %lld", nbFrames);
        return nbFrames * 1.0 / _videoDecoder->getFps();
    }
    return MAXFLOAT;
}
    
void Player::open(const string &path, OpenCallback callback) {
    _curPath = path;
    
    _isOpened = false;
    _openCallback = callback;
    _openThread = new thread([&]() -> void {
        open();
        performInMainThread(openedInMainThread, this);
    });
    
    return;
}

void Player::openedInMainThread(void *param) {
    Player* self = reinterpret_cast<Player *>(param);
    self->opened();
}
    
void Player::opened() {
    if (!_openError.empty()) {
        _openCallback(_openError);
        return;
    }
    
    start();
    
    if (_videoFile.hasAudio()) {
        _audioDecoder->open(_videoFile.getAudioCodec(), _videoFile.getAudioCodecId());
        _audio.init(_audioDecoder, this);
        _audio.open(_audioDecoder->getOutputChannels(), _audioDecoder->getOutputSampleRate());
    }
    
    if (_videoFile.hasVideo()) {
        _videoDecoder->open(_videoFile.getVideoCodec(), _videoFile.getVideoCodecId());
        _video.init(_videoDecoder, this);
        //确保宽和高都是4的倍数，ffmpeg要求是2的倍数，OpenGL要求是2的倍数，由于yuv420还要除以2，所以要是4的倍数
        _video.open(_videoDecoder->getWidth()/4*4, _videoDecoder->getHeight()/4*4);
    }
    
    _isOpened = true;
    _openCallback(_openError);
    return;
}

void Player::open() {
    try {
        _openError = "";
        _videoFile.open(_curPath);
    } catch (const exception &e) {
        _openError = e.what();
    }
    return;
}

}
