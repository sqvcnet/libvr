#ifndef __AUDIO_H__
#define __AUDIO_H__
#include "platform/SL.h"
#include "std.h"
#include "AudioOutputDelegate.h"

namespace geeek {
    
typedef pair<int64_t, string> PtsFrame;

class AudioDelegate {
public:
    virtual PtsFrame getAudio() = 0;
};

class Audio: public AudioOutputDelegate {
    const int FRAME_BUFFER_SIZE = 16;
public:
    Audio();
    virtual ~Audio();

    void init(AudioDelegate *delegateAudio, SyncDelegate *delegateSync);
    void open(int channels, int sampleRate);
    void start();
    void pause();
    void stop();
    void close();
    void notify();
    int64_t getCurPts();
    virtual void consumeFrames(int count);
    virtual bool getFrame(char **data, size_t *size);
    
private:
    void flushSafe();
    void produceFrame(PtsFrame &&frame);
    void getFrame(PtsFrame **frame);
    void consumeFrame();
    
private:
    AudioDelegate *_delegateAudio;
    SyncDelegate *_delegateSync;
    volatile bool _isExitThread;
    volatile int _got;
    int64_t _curPts;
    thread *_audioThread;
    int _channels;
    int _sampleRate;
    
    list<PtsFrame> _frames;
    mutex _mutexFrames;
    mutex _mutexCv;
    condition_variable _cv;
#if CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID
    OpenSL _audioOutput;
#else
    AudioQueue _audioOutput;
#endif
};
    
}

#endif  // __RENDERER_H__
