//
//  PacketReader.cpp
//  vrplayer
//
//  Created by 单强 on 2017/3/12.
//  Copyright © 2017年 Facebook. All rights reserved.
//

#include "PacketReader.hpp"
#include "Player.hpp"

namespace geeek {
    
PacketReader::PacketReader(Player *player, VideoFile *videoFile, Audio *audio, Video *video)
: _isExitThread(false)
, _isEnd(false)
, _readerThread(nullptr)
, _audioDts(AV_NOPTS_VALUE)
, _videoDts(AV_NOPTS_VALUE)
, _player(player)
, _videoFile(videoFile)
, _audio(audio)
, _video(video) {

}
    
PacketReader::~PacketReader() {
    stop();
}

double PacketReader::getCacheProgress() {
    if (_isEnd) {
        return 1.0;
    }
    return _videoFile->getProgress(_videoDts, _audioDts);
}

int PacketReader::seek(double percent) {
    if (_videoFile->hasVideo()) {
        return _videoFile->seekVideo(percent);
    }
    
    if (_videoFile->hasAudio()) {
        return _videoFile->seekAudio(percent);
    }
    
    return -1;
}

void PacketReader::freePackets(queue<AVPacket*> *packets) {
    while (!packets->empty()) {
        AVPacket *packet = packets->front();
        av_free_packet(packet);
        delete packet;
        packets->pop();
    }
}

void PacketReader::stop() {
    _isExitThread = true;
    if (_readerThread) {
        _readerThread->join();
        delete _readerThread;
        _readerThread = nullptr;
        LOGD("PacketReader thread has been joined");
    }
    
    //the reader frame thread has been stoped, but the audio decoder and video decoder threads may be also run
    std::lock_guard<mutex> lockA(_mutexAudioPackets);
    std::lock_guard<mutex> lockV(_mutexVideoPackets);
    freePackets(&_audioPackets);
    freePackets(&_videoPackets);
    
    _audioDts = AV_NOPTS_VALUE;
    _videoDts = AV_NOPTS_VALUE;
    _isEnd = false;
}
    
bool PacketReader::isEnd() {
    return _isEnd && (_audioPackets.empty() || _videoPackets.empty());
}
    
void PacketReader::start() {
    _audioDts = AV_NOPTS_VALUE;
    _videoDts = AV_NOPTS_VALUE;
    
    LOGD("start read frame thread...");
    _isExitThread = false;
    _isEnd = false;
    _readerThread = new thread([&]()->bool {
        bool retValue = true;
        while (!_isExitThread && !_isEnd) {
            while (!_isExitThread && !_isEnd && !atLeast()) {
                AVPacket *packet = av_packet_alloc();
                int ret = _videoFile->readFrame(packet);
                switch (ret) {
                    case 0:
                        if (_videoFile->isAudio(packet->stream_index)) {
                            _audioDts = packet->dts;
                            produceAudioPacket(packet);
                        } else if (_videoFile->isVideo(packet->stream_index)) {
                            _videoDts = packet->dts;
                            produceVideoPacket(packet);
                        } else {
                            LOGE("Neither audio packet nor video packet\n");
                        }
                        break;
                    case -1:
                        //eof
                        _isEnd = true;
                        break;
                    default:
                        //error
                        _player->setLastError(Player::Error::READ_FRAME_ERROR);
                        //另一个线程将_lastError取走后会将它赋值为0，这会导致失败情况下外层循环依然有机会执行到
                        //失败情况下的继续执行可能会导致crash，帮直接 return true;
                        return true;
                }
            }
            
            unique_lock<mutex> uniqueLock(_mutexCv);
            _cv.wait_for(uniqueLock, chrono::milliseconds(50));
        }
        return true;
    });
    
    return;
}

bool PacketReader::atLeast() {
    const int ATLEAST = 300;
    if (_videoFile->hasAudio() && _videoFile->hasVideo()) {
        return (_audioPackets.size() >= ATLEAST && _videoPackets.size() >= ATLEAST);
    } else if (_videoFile->hasAudio()) {
        return (_audioPackets.size() >= ATLEAST);
    } else if (_videoFile->hasVideo()) {
        return (_videoPackets.size() >= ATLEAST);
    } else {
        return true;
    }
}

void PacketReader::produceAudioPacket(AVPacket *packet) {
    std::lock_guard<mutex> lock(_mutexAudioPackets);
    _audioPackets.push(packet);
    _audio->notify();
}

void PacketReader::consumeAudioPacket(AVPacket **packet) {
    std::lock_guard<mutex> lock(_mutexAudioPackets);
    if (!_audioPackets.empty()) {
        *packet = _audioPackets.front();
        _audioPackets.pop();
    } else {
        *packet = nullptr;
    }
    _cv.notify_one();
}

void PacketReader::produceVideoPacket(AVPacket *packet) {
    std::lock_guard<mutex> lock(_mutexVideoPackets);
    _videoPackets.push(packet);
    _video->notify();
}

void PacketReader::consumeVideoPacket(AVPacket **packet) {
    std::lock_guard<mutex> lock(_mutexVideoPackets);
    if (!_videoPackets.empty()) {
        *packet = _videoPackets.front();
        _videoPackets.pop();
    } else {
        *packet = nullptr;
    }
    _cv.notify_one();
}
    
}
