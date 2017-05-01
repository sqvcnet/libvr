//
//  OpenSL.hpp
//  vrplayer
//
//  Created by 单强 on 16/7/27.
//  Copyright © 2016年 Facebook. All rights reserved.
//

#ifndef OpenSL_hpp
#define OpenSL_hpp

#include "Util.h"
#include "AudioOutputDelegate.h"

#include <SLES/OpenSLES.h>

class OpenSL {
    const SLuint32 OPENSL_BUFFERS = 1;
    static const int SILENCE_SIZE = 4096;
    const char SILENCE[SILENCE_SIZE+4] = {0};
    const char *SILENCE4PACK = (char *)(((unsigned long long)SILENCE+3) & (~0x03));

public:
    OpenSL();
    ~OpenSL();
    void uninit();
    SLresult init(AudioOutputDelegate *delegate);
    SLresult open(SLuint32 channels, SLuint32 sampleRate);
    bool isOpened();
    void close();
    
private:
    static void playCallback(SLBufferQueueItf caller, void *context);
    bool writeBuffer(const char *data, int size);
    
private:
    // engine interfaces
    SLObjectItf _engineObject;
    SLEngineItf _engineEngine;

    // output mix interfaces
    SLObjectItf _outputMixObject;
    SLPresetReverbItf _outputMixPresetReverb;

    SLObjectItf _bqPlayerObject;
    SLPlayItf _bqPlayerPlay;
    SLBufferQueueItf _bqPlayerBufferQueue;
    SLEffectSendItf _bqPlayerEffectSend;
    SLVolumeItf _bqPlayerVolume;
    
    AudioOutputDelegate *_delegate;
    volatile bool _isExitThread;
    volatile bool _isClosed;
    int _lastCount;
};

#endif /* OpenSL_hpp */
