//
//  Video.cpp
//  vrplayer
//
//  Created by 单强 on 16/7/27.
//  Copyright © 2016年 Facebook. All rights reserved.
//

#include "Video.hpp"

namespace geeek {
    
Video::Video()
:_ringQueue{0}
,_ringQueueFront(0)
,_ringQueueRear(0)
,_ringQueueEmpty(true)
,_delegateVideo(nullptr)
,_delegateSync(nullptr)
,_isExitThread(true)
,_isGot(false)
,_videoThread(nullptr)
,_dstFrameWidth(0)
,_dstFrameHeight(0)
,_dstPixFmt(Renderer::PixFmt::YUV420P)
,_isUseFrame(true)
//,_dstPixFmt(AV_PIX_FMT_NV12)
{
    for (int i = 0; i < TEXTURE_BUFFER_SIZE; i++) {
        _ringQueue[i].frame = nullptr;
        for (int j = 0; j < 4; j++) {
            _ringQueue[i].data[j] = nullptr;
            _ringQueue[i].linesize[j] = 0;
        }
    }
}

Video::~Video() {
    release();
    LOGD("Video::~Video");
}

void Video::init(VideoDelegate *delegateVideo, SyncDelegate *delegateSync) {
    _delegateVideo = delegateVideo;
    _delegateSync = delegateSync;
    _renderer.setDelegate(this);
}

Renderer *Video::getRenderer() {
    return &_renderer;
}

void Video::notify() {
    _cv.notify_one();
}

bool Video::getTexture(unsigned char **dataY, unsigned char **dataU, unsigned char **dataV, Renderer::PixFmt *pixFmt) {
    *dataY = nullptr;
    *dataU = nullptr;
    *dataV = nullptr;
    
    *pixFmt = _dstPixFmt;
    
    uint8_t *yuvData[4];
    if (!getTexture(yuvData)) {
        return false;
    }
    
    *dataY = yuvData[0];
    *dataU = yuvData[1];
    *dataV = yuvData[2];

    return true;
}
    
void Video::pause() {
    _isExitThread = true;
    if (_videoThread) {
        _videoThread->join();
        delete _videoThread;
        _videoThread = nullptr;
        LOGD("Video has been joined");
    }
    LOGD("Video has been paused");
}
    
void Video::stop() {
    pause();
    flush();
    LOGD("Video has been stoped");
}
    
void Video::close() {
    stop();
    _renderer.close();
    release();
}
    
void Video::alloc() {
    release();
    for (int i = 0; i < TEXTURE_BUFFER_SIZE; i++) {
        _ringQueue[i].frame = av_frame_alloc();
        for (int j = 0; j < 4; j++) {
            _ringQueue[i].data[j] = nullptr;
            _ringQueue[i].linesize[j] = 0;
        }
    }
}

void Video::release() {
    for (int i = 0; i < TEXTURE_BUFFER_SIZE; i++) {
        av_frame_free(&(_ringQueue[i].frame));
        if (_ringQueue[i].data[0] != nullptr) {
            av_freep(&_ringQueue[i].data[0]);
            _ringQueue[i].data[0] = nullptr;
        }
    }
}
    
void Video::open(int width, int height) {
    _dstFrameWidth = width;
    _dstFrameHeight = height;
    alloc();
    _renderer.open(width, height);
}

void Video::flush() {
    LOGD("Video::consumeTexture flush");
    _ringQueueFront = _ringQueueRear = 0;
    _ringQueueEmpty = true;
    LOGD("Video::consumeTexture flushed");
}

//这个函数可以不用停线程直接flush
//void Video::flush() {
//    lock_guard<mutex> lock(_mutexTextures);
//    //只有Renderer是open的状态时才有可能正在加载一个texture,这边释放才会导致crash
//    if (_renderer.isOpened()) {
//        //确保renderer.render被持续调到
//        //open时会释放YUVBuffer然后重新分配，在这之前必须完成flush，否则renderer刚load的texture可能还在load中，这边释放会导致crash
//        //必须在stop之后调用，否则刚flush完立刻又会生产进去
//        //必须在flush完之后才可以停renderer，否则永远等不到_isFlushing为false
//#if CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID // || CC_TARGET_PLATFORM == CC_PLATFORM_IOS
////        _isFlushing = true;
////        while (_isFlushing) {
////            this_thread::sleep_for(chrono::milliseconds(50));
////            LOGD("Video flushing...");
////        }
//        _renderer.close();
//        flushUnSafe();
//        _renderer.open(_dstFrameWidth, _dstFrameHeight);
//#else
//        flushUnSafe();
//#endif
//    } else {
//        flushUnSafe();
//    }
//}

void Video::start() {
    if (_isExitThread == false) {
        return;
    }
    _isGot = false;
    _isExitThread = false;
    _videoThread = new thread([&]()->void {
        while (!_isExitThread) {
            int isErr = false;
            while (!_isExitThread && !isRingQueueFull()) {
                AVPixelFormat pixFmt = AV_PIX_FMT_NONE;
                int ret = _delegateVideo->getVideo(&pixFmt, &(_ringQueue[_ringQueueRear].pts),
                                         _ringQueue[_ringQueueRear].frame, _ringQueue[_ringQueueRear].data, _ringQueue[_ringQueueRear].linesize);
                if (ret == 1 || ret == 2) {
                    switch (pixFmt) {
                        case AV_PIX_FMT_YUV420P:
                            _dstPixFmt = Renderer::PixFmt::YUV420P;
                            break;
                        case AV_PIX_FMT_NV12:
                            _dstPixFmt = Renderer::PixFmt::NV12;
                            break;
                        case AV_PIX_FMT_NV21:
                            _dstPixFmt = Renderer::PixFmt::NV21;
                            break;
                        case AV_PIX_FMT_RGB24:
                            _dstPixFmt = Renderer::PixFmt::RGB;
                            break;
                        default:
                            LOGE("Video::start Unsupported pixel format!");
                            break;
                    }
                    if (ret == 1) {
                        _isUseFrame = true;
                    } else {
                        _isUseFrame = false;
                    }
                    produceTexture();
                } else if (ret == 0) {
                    break;
                } else if (ret < 0) {
                    isErr = true;
                    break;
                }
            }
            if (isErr) {
                break;
            }

            unique_lock<mutex> uniqueLock(_mutexCv);
            _cv.wait_for(uniqueLock, chrono::milliseconds(50));
        }
        return;
    });
    
    return;
}

bool Video::isRingQueueFull() {
    return (_ringQueueFront == _ringQueueRear && !_ringQueueEmpty);
}

void Video::produceTexture() {
    lock_guard<mutex> lock(_mutexTextures);
    _ringQueueRear = (_ringQueueRear+1) % TEXTURE_BUFFER_SIZE;
    _ringQueueEmpty = false;
}

bool Video::getTexture(uint8_t *yuvData[4]) {
    lock_guard<mutex> lock(_mutexTextures);
    if (!_ringQueueEmpty) {
        if (_delegateSync->computeVideoClock(_ringQueue[_ringQueueFront].pts) <= _delegateSync->computeAudioClock(AV_NOPTS_VALUE)) {
            if (_isUseFrame) {
                for (int i = 0; i < 4; i++) {
                    yuvData[i] = _ringQueue[_ringQueueFront].frame->data[i];
                }
            } else {
                for (int i = 0; i < 4; i++) {
                    yuvData[i] = _ringQueue[_ringQueueFront].data[i];
                }
            }
            _isGot = true;
        } else {
//            LOGD("getTexture: too early");
        }
    } else {
        if (!_isExitThread) {
//            LOGD("getTexture: null texture");
        }
    }
    return _isGot;
}

void Video::consumeTexture() {
    lock_guard<mutex> lock(_mutexTextures);
    if (!_ringQueueEmpty && _isGot) {
//        av_frame_unref(_ringQueue[_ringQueueFront].frame);
        _ringQueueFront = (_ringQueueFront+1) % TEXTURE_BUFFER_SIZE;
        if (_ringQueueFront == _ringQueueRear) {
            _ringQueueEmpty = true;
        }
        _isGot = false;
        notify();
    }
}

}
