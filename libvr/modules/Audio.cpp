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
,_isFlushing(false)
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
    _audioOutput.close();
    LOGD("Audio has been stoped");
}

void Audio::pause() {
    _isExitThread = true;
    if (_audioThread) {
        _audioThread->join();
        delete _audioThread;
        _audioThread = nullptr;
        LOGD("Audio has been joined");
    }
}

//该函数暂时没用，以后可能会用到，这个函数可以不用停线程直接flush
void Audio::flush() {
    //必须在stop之后调用，否则刚flush完立刻又会生产进去
    //必须在flush完之后才可以停audioOutput线程(_audioOutput.close())，否则永远等不到_isFlushing为false
    _mutexFrames.lock();
    if (_audioOutput.isOpened()) {
        _mutexFrames.unlock();
        _isFlushing = true;
        while (_isFlushing) {
            this_thread::sleep_for(chrono::milliseconds(50));
            LOGD("Audio flushing...");
        }
    } else {
        flushUnSafe();
        _mutexFrames.unlock();
    }
    LOGD("Audio flushed");
}

void Audio::flushUnSafe() {
    list<PtsFrame> empty;
    swap(_frames, empty);
    _isFlushing = false;
}

void Audio::close() {
    stop();
    //stop会等待到真正结束本线程，并且等待到真正结束声音线程，所以可以不加锁的直接flushUnSafe
    flushUnSafe();
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
//    _audioOutput.open(_channels, _sampleRate);
    LOGD("Audio has been opened: channels: %d, sampleRate: %d", _channels, _sampleRate);
}

int64_t Audio::getCurPts() {
    return _curPts;
}

void Audio::consumeFrames(int count) {
    _mutexFrames.lock();
    if (_isFlushing) {
        flushUnSafe();
    }
    _mutexFrames.unlock();
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
    if (_isExitThread == false) {
        return;
    }
    _isExitThread = false;
    
    _audioOutput.open(_channels, _sampleRate);
    LOGD("AudioOutput has been opened: channels: %d, sampleRate: %d", _channels, _sampleRate);
    
    _mutexFrames.lock();
    _got = 0;
    _mutexFrames.unlock();
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
    _mutexFrames.lock();
    _frames.push_back(std::move(frame));
    _mutexFrames.unlock();
}

void Audio::getFrame(PtsFrame **frame) {
    _mutexFrames.lock();
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
    _mutexFrames.unlock();
}

void Audio::consumeFrame() {
    _mutexFrames.lock();
    if (!_frames.empty() && _got > 0) {
        _frames.pop_front();
        _got--;
        notify();
    }
    _mutexFrames.unlock();
}

}
