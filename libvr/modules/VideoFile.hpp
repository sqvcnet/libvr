//
//  VideoFile.hpp
//  vrplayer
//
//  Created by 单强 on 16/5/20.
//  Copyright © 2016年 单强. All rights reserved.
//

#ifndef VideoFile_hpp
#define VideoFile_hpp

#include "std.h"

namespace geeek {
    
class VideoFile {
public:
    VideoFile();
    virtual ~VideoFile();

    void open(const string & srcPath);
    void close();
    void abortOpen();
    
    int readFrame(AVPacket *packet);

    double getProgress(int64_t videoDts, int64_t audioDts);
    double getCacheProgress();
    double getTotalTime();
    
    bool hasAudio() const;
    bool hasVideo() const;
    
    AVCodecContext* getAudioCodec() const;
    AVCodecContext* getVideoCodec() const;
    
    enum AVCodecID getAudioCodecId() const;
    enum AVCodecID getVideoCodecId() const;
    
    int seekAudio(double percent);
    int seekVideo(double percent);
    
    bool isAudio(int streamIndex) const;
    bool isVideo(int streamIndex) const;
  
private:
    static int openCb(void *p);
    int openAudioOrVideo(int stream_index);
    double getExternalClock();
    double ts2clock(int64_t ts, int64_t startTs, AVRational timebase);
    
private:
    AVFormatContext *_avFormatCtx;
    int _videoStreamIndex;
    int _audioStreamIndex;
    AVCodecContext* _audioCodec;
    AVCodecContext* _videoCodec;
    enum AVCodecID _audioCodecId;
    enum AVCodecID _videoCodecId;
    int _openRet;
};

}

#endif /* VideoFile_hpp */
