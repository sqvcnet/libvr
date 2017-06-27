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
    enum class Error {NO_ERROR = 0, DECODE_ERROR = -1, READ_FRAME_ERROR = -3};
    using OpenCallback = function<void(const string &)>;
    
public:
    Player();
    virtual ~Player();
    
    Renderer *getRenderer();
    void open(const string &path, OpenCallback callback);
    void play();
    void pause();
    void setCodec(int codec);
    void seek(double percent);
    void close();
    double getProgress();
    void setLastError(Error err);
    Error getLastError();
    double getCacheProgress();
    float getTotalTime();
    
    virtual double computeAudioClock(int64_t nbSamples);
    virtual double computeVideoClock(int64_t nbFrames);
    
private:
    void open();
    void opened();
    static void openedInMainThread(void *param);
    void readyPlay();
    void start();
    void stop();
    void openDecoder();
    void closeDecoder();
    
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
    bool _isPlaying;
    string _curPath;
    thread *_openThread;
    int _srcFrameWidth;
    int _srcFrameHeight;
    int _dstFrameWidth;
    int _dstFrameHeight;
    string _decodedJpgPath;
    double _startPlayTime;
    Error _lastError;
    string _openError;
    OpenCallback _openCallback;
};
    
}//namespace geeek

#endif /* Player_hpp */
