//
//  Video.hpp
//  vrplayer
//
//  Created by 单强 on 16/7/27.
//  Copyright © 2016年 Facebook. All rights reserved.
//

#ifndef Video_hpp
#define Video_hpp

#include "std.h"
#include "Renderer.h"

namespace geeek {
    
class VideoDelegate {
public:
//    virtual int getVideo(AVPixelFormat dstPixFmt, int64_t *pts, uint8_t *yuvData[4], int yuvLineSize[4]) = 0;
    virtual int getVideo(AVPixelFormat *pixFmt, int64_t *pts, AVFrame *frame, uint8_t *data[4], int linesize[4]) = 0;
};

typedef struct _PtsAVFrame {
    int64_t pts;
    AVFrame *frame;
    uint8_t *data[4];
    int linesize[4];
} PtsAVFrame;

class Video: public RendererDelegate {
    static const int TEXTURE_BUFFER_SIZE = 6;
public:
    Video();
    ~Video();
    void init(VideoDelegate *delegateVideo, SyncDelegate *delegateSync);
    Renderer *getRenderer();
    void notify();
    void start();
    void stop();
    void open(int width, int height);
    void close();
    virtual bool getTexture(unsigned char **dataY, unsigned char **dataU, unsigned char **dataV, Renderer::PixFmt *pixFmt);
    virtual void consumeTexture();
    
private:
    void flush();
    void flushUnSafe();
    void produceTexture();
    bool getTexture(uint8_t *yuvData[4]);
    bool isRingQueueFull();
    
private:
    PtsAVFrame _ringQueue[TEXTURE_BUFFER_SIZE];
    int _ringQueueFront;
    int _ringQueueRear;
    bool _ringQueueEmpty;
    mutex _mutexTextures;
    mutex _mutexCv;
    condition_variable _cv;
    
    Renderer _renderer;
    
    VideoDelegate *_delegateVideo;
    SyncDelegate *_delegateSync;
    volatile bool _isExitThread;
    volatile bool _isGot;
    volatile bool _isFlushing;
    thread *_videoThread;
    int _dstFrameWidth;
    int _dstFrameHeight;
    Renderer::PixFmt _dstPixFmt;
    bool _isUseFrame;
};

}

#endif /* Video_hpp */
