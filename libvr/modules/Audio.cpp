#include "Audio.h"
#include "Util.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <assert.h>

#include <thread>
#include <string>
#include <vector>
#include <fstream>
#include <iostream>
using namespace std;

namespace geeek {
    
Audio::Audio()
:_delegateAudio(nullptr)
,_delegateSync(nullptr)
,_isExitThread(true)
,_got(0)
,_curPts(AV_NOPTS_VALUE)
,_audioThread(nullptr)
,_channels(0)
,_sampleRate(0)
{
    
}

Audio::~Audio() {
    
}

void Audio::stop() {
    pause();
    flushSafe();
    LOGD("Audio has been stoped");
}

void Audio::pause() {
    //mute and don't consume audio frames firstly
    _audioOutput.close();
    //stop to produce audio frames secondly
    _isExitThread = true;
    if (_audioThread) {
        _audioThread->join();
        delete _audioThread;
        _audioThread = nullptr;
        LOGD("Audio has been joined");
    }
}

void Audio::flushSafe() {
    lock_guard<mutex> lock(_mutexFrames);
    list<PtsFrame> empty;
    swap(_frames, empty);
}

void Audio::close() {
    stop();
    LOGD("Audio has been closed");
}

void Audio::init(AudioDelegate *delegateAudio, SyncDelegate *delegateSync) {
    _delegateAudio = delegateAudio;
    _delegateSync = delegateSync;
    _audioOutput.init(this);
}

void Audio::open(int channels, int sampleRate) {
    _channels = channels;
    _sampleRate = sampleRate;
    LOGD("Audio has been opened: channels: %d, sampleRate: %d", _channels, _sampleRate);
}

int64_t Audio::getCurPts() {
    return _curPts;
}

void Audio::consumeFrames(int count) {
    while (count > 0 && _got > 0) {
        consumeFrame();
        count--;
    }
}

bool Audio::getFrame(char **data, size_t *size) {
    *data = nullptr;
    *size = 0;
    
    PtsFrame *frame = nullptr;
    getFrame(&frame);
    
    if (nullptr == frame) {
        return false;
    }
    
    *data = const_cast<char *>(frame->second.data());
    *size = frame->second.size();
    
    return true;
}

void Audio::notify() {
    _cv.notify_one();
}

void Audio::start() {
    lock_guard<mutex> lock(_mutexFrames);
    if (_isExitThread == false) {
        return;
    }
    _isExitThread = false;
    
    _audioOutput.open(_channels, _sampleRate);
    LOGD("AudioOutput has been opened: channels: %d, sampleRate: %d", _channels, _sampleRate);
    
    _got = 0;
    _curPts = AV_NOPTS_VALUE;
    _audioThread = new thread([&]()->void {
        while (!_isExitThread) {
            while (!_isExitThread && _frames.size() < FRAME_BUFFER_SIZE) {
                PtsFrame frame = _delegateAudio->getAudio();
                if (frame.second.empty()) {
                    break;
                }
                produceFrame(std::move(frame));
            }
        
            unique_lock<mutex> uniqueLock(_mutexCv);
            _cv.wait_for(uniqueLock, chrono::milliseconds(50));
        }
        return;
    });
    
    return;
}

void Audio::produceFrame(PtsFrame &&frame) {
    lock_guard<mutex> lock(_mutexFrames);
    _frames.push_back(std::move(frame));
}

void Audio::getFrame(PtsFrame **frame) {
    lock_guard<mutex> lock(_mutexFrames);
    *frame = nullptr;
    if (!_frames.empty() && _frames.size() > _got) {
        list<PtsFrame>::iterator it = _frames.begin();
        std::advance(it, _got);
        if (_delegateSync->computeAudioClock(it->first) <= _delegateSync->computeVideoClock(AV_NOPTS_VALUE)) {
            *frame = &(*it);
            _got++;
            _curPts = (*frame)->first;
        } else {
//            LOGD("getFrame: too early, _got: %d", _got);
        }
    } else {
//        LOGD("getFrame: null frame, _got: %d", _got);
    }
}

void Audio::consumeFrame() {
    lock_guard<mutex> lock(_mutexFrames);
    if (!_frames.empty() && _got > 0) {
        _frames.pop_front();
        _got--;
        notify();
    }
}

}
