//
//  AudioQueue.hpp
//  vrplayer
//
//  Created by 单强 on 16/7/27.
//  Copyright © 2016年 Facebook. All rights reserved.
//

#ifndef AudioQueue_hpp
#define AudioQueue_hpp

#include "std.h"
#include "AudioOutputDelegate.h"
#import <AudioToolbox/AudioQueue.h>
#import <AudioToolbox/AudioSession.h>

class AudioQueue {
    static const int BUFFER_NUM = 30;
    
public:
    AudioQueue();
    void init(AudioOutputDelegate *delegate);
    int open(int channels, int sampleRate);
    bool isOpened();
    void close();
  
private:
    void determineOutputDevice();
    static void audioQueueCallback(void * inUserData, AudioQueueRef inAQ, AudioQueueBufferRef inBuffer);
    static void interruptionListener(void *inClientData, UInt32 inInterruptionState);
    static void propListener(void *inClientData, AudioSessionPropertyID inID, UInt32 inDataSize, const void *inData);
    void play(AudioQueueBufferRef outBuffer);
    void allocBuffers();
    void freeBuffers();
    
    AudioOutputDelegate *_delegate;
    thread *_audioThread;
    volatile bool _isExitThread;
    AudioQueueRef _audioQueueRef;
    int _bufSize;
    AudioQueueBufferRef _outBuffers[BUFFER_NUM];
    int _channels;
    int _sampleRate;
};

#endif /* AudioQueue_hpp */
