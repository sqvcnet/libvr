//
//  Player.hpp
//  vrplayer
//
//  Created by 单强 on 16/6/14.
//  Copyright © 2016年 Facebook. All rights reserved.
//

#ifndef Player_hpp
#define Player_hpp

#include "std.h"

#include "Util.h"
#include "Audio.h"
#include "Video.hpp"
#include "Renderer.h"
#include "Image.hpp"
#include "PacketReader.hpp"
#include "AudioDecoder.hpp"
#include "VideoDecoder.hpp"

namespace geeek {
    
class Player: public SyncDelegate {
public:
    Player(void *playerView);
    virtual ~Player();
    
    Renderer *getRenderer();
    bool open(const string &path);
    void reopen();
    void play();
    void pause();
    void setCodec(int codec);
    void seek(double percent);
    void close();
    double getProgress();
    void setLastError(int err);
    int getLastError();
    double getCacheProgress();
    float getTotalTime();
    
    virtual double computeAudioClock(int64_t nbSamples);
    virtual double computeVideoClock(int64_t nbFrames);
    
private:
    bool open();
    bool opened();
    static void openedInMainThread(void *param);
    static void stop(void *self);
    static void start(void *self);
    
    static void inMainThread(void *param);
    void pauseRenderer(bool isPause);
    
private:
    VideoFile _videoFile;
    PacketReader *_packetReader;
    AudioDecoder *_audioDecoder;
    VideoDecoder *_videoDecoder;
    Audio _audio;
    Video _video;
    
    int64_t _audioCurNbSamples;
    int64_t _videoCurNbFrames;
    
    AVPixelFormat _srcPixFmt;
    AVPixelFormat _dstPixFmt;
    
    volatile bool _isOpened;
    string _curPath;
    thread *_openThread;
    int _srcFrameWidth;
    int _srcFrameHeight;
    int _dstFrameWidth;
    int _dstFrameHeight;
    string _decodedJpgPath;
    double _startPlayTime;
    void *_playerView;
    double _seek;
    bool _needPlay;
    int _lastError;
};
    
}//namespace geeek

#endif /* Player_hpp */
