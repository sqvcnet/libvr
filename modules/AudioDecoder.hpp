//
//  AudioDecoder.hpp
//  vrplayer
//
//  Created by 单强 on 2017/3/12.
//  Copyright © 2017年 Facebook. All rights reserved.
//

#ifndef AudioDecoder_hpp
#define AudioDecoder_hpp

#include "std.h"
#include "Audio.h"

namespace geeek {

class PacketReader;
class AudioDecoder : public AudioDelegate {
public:
    AudioDecoder(PacketReader *_packetReader);
    ~AudioDecoder();
    
    int64_t getCurDts();
    int getOutputChannels();
    int getSampleRate();
    int getOutputSampleRate();
    int open(AVCodecContext* audioCodec, enum AVCodecID codecId);
    void close();
    
    virtual PtsFrame getAudio();
    
private:
    bool allocAudioBuffer(int nbSamples);
    void freeAudioBuffer();
    
private:
    PacketReader *_packetReader;
    AVFrame *_audioFrame;
    AVCodecContext *_audioCtx;
    struct SwrContext *_swrCtx;
    int _outSampleRate;
    int64_t _curDts;
    
    unsigned char **_dstAudioData;
    int _srcNbSamples;
    int _dstNbSamples;
    AVSampleFormat _dstSampleFmt;
    int64_t _audioNbSamples;
    int64_t _outChannels;
};

}

#endif /* AudioDecoder_hpp */
