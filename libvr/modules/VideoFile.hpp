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
    
    bool hasAudio();
    bool hasVideo();
    
    AVCodecContext* getAudioCodec();
    AVCodecContext* getVideoCodec();
    
    enum AVCodecID getAudioCodecId();
    enum AVCodecID getVideoCodecId();
    
    int seekAudio(double percent);
    int seekVideo(double percent);
    
    bool isAudio(int streamIndex);
    bool isVideo(int streamIndex);
  
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
