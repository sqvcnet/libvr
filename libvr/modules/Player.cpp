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
    
Player::Player(void *playerView)
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
,_seek(-1.0)
,_needPlay(false)
,_lastError(0)
{
    _startPlayTime = av_gettime()/1000000.0;
    _playerView = playerView;
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

void Player::pauseRenderer(bool isPause) {
#if CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID
    ::pauseRenderer(_playerView, isPause);
#endif
    return;
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
    if (string::npos != _curPath.find(".jpg")) {
        _video.close();
        _decodedJpgPath = "";
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
    _needPlay = false;
    _seek = -1.0;
    _lastError = 0;
    _playerView = nullptr;
}

void Player::pause() {
    LOGD("Player:pause start: %lld", av_gettime());
    _audio.stop();
    _video.stop();
    LOGD("Player:pause end: %lld", av_gettime());
//    pauseRenderer(true);
    LOGD("Player has been paused");
}

void Player::setCodec(int codec) {
    if (codec != _videoDecoder->getCodec()) {
        _videoDecoder->setCodec(codec);
    }
}

void Player::seek(double percent) {
    if (string::npos != _curPath.find(".jpg")) {
        return;
    }
    if (/*percent <= 1e-6 ||
        getProgress() <= 1e-6 ||*/
        abs(percent - getProgress()) <= 1e-6) {
        return;
    }
    LOGD("Player::seek start %f, progress: %f", percent, getCacheProgress());
    if (_isOpened) {
        //只有open的情况下，Renderer::render才会走到loadTexture，否则_video.close会死等，audio同理
//        pause();
        
//        _audio.close();
//        _video.close();
//        flushAudioPackets();
//        flushVideoPackets();
        close();
        
        //_videoFile.seek(percent);
        _seek = percent;
        
        open(_curPath);
//        _audioNbSamples = 0;
//        _videoNbFrames = 0;
//        _audioCurNbSamples = 0;
//        _videoCurNbFrames = 0;
//        _videoDts = AV_NOPTS_VALUE;
//        _audio.open(_videoFile.getChannels(), _outSampleRate);
//        _video.open(_dstFrameWidth, _dstFrameHeight);
        //        play();
    } else {
        _seek = percent;
    }
    LOGD("Player::seek end %f, progress: %f", percent, getCacheProgress());
}
    
int Player::getLastError() {
    int err = _lastError;
    _lastError = 0;
    return err;
}

void Player::setLastError(int err) {
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
            double syncGap = 1.0 * _videoDecoder->getFps();
            return 1.0 * _audioCurNbSamples / outRate + syncGap;
        } else {
            _audioCurNbSamples = nbSamples;
        }
//        LOGD("nbSamples: %lld", nbSamples);
        return 1.0 * nbSamples / outRate;
    }
    return av_gettime()/1000000.0 - _startPlayTime;
}

double Player::computeVideoClock(int64_t nbFrames) {
    if (string::npos != _curPath.find(".jpg")) {
        return AV_NOPTS_VALUE;
    }
    if (_videoFile.hasVideo()) {
        if (nbFrames == AV_NOPTS_VALUE) {
            //audio 最快可以比 video 快2帧，超过2帧播放静音来等
            nbFrames = _videoCurNbFrames + 2;
        } else {
            _videoCurNbFrames = nbFrames;
        }
    //    LOGD("nbFrames: %lld", nbFrames);
        return 1.0 * nbFrames * _videoDecoder->getFps();
    }
    return MAXFLOAT;
}

void Player::stop(void *self) {
    Player *player = reinterpret_cast<Player *>(self);
    player->_audio.stop();
    player->_video.stop();
}

void Player::start(void *self) {
    Player *player = reinterpret_cast<Player *>(self);
    player->_audio.start();
    player->_video.start();
}

void Player::play() {
    if (_isOpened) {
        _audio.start();
        _video.start();
        _startPlayTime = av_gettime()/1000000.0 - _videoCurNbFrames * _videoDecoder->getFps();
    } else {
        _needPlay = true;
    }
    LOGD("Player::play");
}
    
bool Player::open(const string &path) {
    if (path.empty()) {
        return false;
    }
    _curPath = path;
    if (string::npos != _curPath.find(".jpg")) {
        //        _decodedJpgPath = "";
        _video.init(_videoDecoder, this);

        //TODO: asynchronize process
        _video.open(3840, 2160);
        _video.start();
        return true;
    }

    _isOpened = false;
    _openThread = new thread([&]()->void {
        if (_isOpened == false) {
            if (!open()) {
                return;
            }
        }
        
        if (_seek >= 0.0) {
            _packetReader->seek(_seek);
            _seek = -1.0;
        }
        
        _isOpened = true;
        performInMainThread(openedInMainThread, this);
    });
    
    return true;
}
    
void Player::openedInMainThread(void *param) {
    Player* self = reinterpret_cast<Player *>(param);
    self->opened();
}
    
bool Player::opened() {
    if (!_isOpened) {
        return false;
    }
    
    _audioCurNbSamples = 0;
    _videoCurNbFrames = 0;
    
    _packetReader->start();
    
    if (_videoFile.hasAudio()) {
        _audioDecoder->open(_videoFile.getAudioCodec(), _videoFile.getAudioCodecId());
        _audio.init(_audioDecoder, this);
        _audio.open(_audioDecoder->getOutputChannels(), _audioDecoder->getOutputSampleRate());
    }
    
    if (_videoFile.hasVideo()) {
        _videoDecoder->open(_videoFile.getVideoCodec(), _videoFile.getVideoCodecId());
        _video.init(_videoDecoder, this);
        //确保宽和高都是4的倍数，ffmpeg要求是2的倍数，OpenGL要求是2的倍数，由于yuv420还要除以2，所以要是4的倍数
        _video.open(_videoDecoder->getWidth(), _videoDecoder->getHeight());
    }

    if (_needPlay) {
        play();
        _needPlay = false;
    }
    
    return true;
}

bool Player::open() {
    int ret = 0;
    if ((ret = _videoFile.open(_curPath)) != 0) {
        LOGD("Player::open failed!!! ret: %d", ret);
        _lastError = ret;
        return false;
    }
    return true;
}

void Player::inMainThread(void *param) {
    Player* player = reinterpret_cast<Player *>(param);
    player->close();
    player->open(player->_curPath);
    player->play();
}

void Player::reopen() {
    _lastError = -1;
//    _videoFile.unsupportHWCodec();
//    performInMainThread(Player::inMainThread, this);
}
    
}
