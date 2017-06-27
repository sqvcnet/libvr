//
//  VideoDecoder.cpp
//  vrplayer
//
//  Created by 单强 on 2017/3/12.
//  Copyright © 2017年 Facebook. All rights reserved.
//

#include "VideoDecoder.hpp"
#include "Player.hpp"
#include "FileUtil.hpp"
#include "PacketReader.hpp"

#if CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID
#else
extern "C" {
#include "libavcodec/videotoolbox.h"
}
#endif

namespace geeek {
    
bool VideoDecoder::_isSupportHevcHWCodec = true;
bool VideoDecoder::_isSupportAvcHWCodec = true;

VideoDecoder::VideoDecoder(Player *player, PacketReader *packetReader)
    : _videoFrame(nullptr)
    , _videoCtx(nullptr)
    , _swsCtx(nullptr)
    , _packetReader(packetReader)
    , _player(player)
    , _canHWAccel(true)
#if CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID
    , _curCodecName("h264_mediacodec")
#endif
    , _curDts(AV_NOPTS_VALUE)
    , _videoNbFrames(0)
    , _srcFrameWidth(0)
    , _srcFrameHeight(0)
    , _dstFrameWidth(0)
    , _dstFrameHeight(0)
    , _srcPixFmt(AV_PIX_FMT_NONE)
    , _dstPixFmt(AV_PIX_FMT_NONE)
    , _srcVideoCtx(nullptr)
    , _srcCodecId(AV_CODEC_ID_NONE)
    , _fps(0.0)
{
    _videoFrame = av_frame_alloc();
}

VideoDecoder::~VideoDecoder() {
    close();
    if (_videoFrame) {
        av_frame_free(&_videoFrame);
        _videoFrame = nullptr;
    }
}

void VideoDecoder::close() {
    if (_videoCtx) {
        avcodec_flush_buffers(_videoCtx);
        videotoolbox_uninit(_videoCtx);
        avcodec_free_context(&_videoCtx);
        _videoCtx = nullptr;
    }
    
    _srcFrameWidth = 0;
    _srcFrameHeight = 0;
    _dstFrameWidth = 0;
    _dstFrameHeight = 0;
    _srcPixFmt = AV_PIX_FMT_NONE;
    _dstPixFmt = AV_PIX_FMT_NONE;
    safeFreeSwsContext();
    
    _curDts = AV_NOPTS_VALUE;
}

int64_t VideoDecoder::getCurDts() {
    return _curDts;
}

int VideoDecoder::getWidth() {
    return _dstFrameWidth;
}

int VideoDecoder::getHeight() {
    return _dstFrameHeight;
}

double VideoDecoder::getFps() {
    return _fps;
}

void VideoDecoder::safeFreeSwsContext() {
    if (_swsCtx) {
        sws_freeContext(_swsCtx);
        _swsCtx = nullptr;
    }
}

void VideoDecoder::unsupportHWCodec() {
    if (_curCodecName == "h265_mediacodec") {
        _isSupportHevcHWCodec = false;
    }
    if (_curCodecName == "h264_mediacodec") {
        _isSupportAvcHWCodec = false;
    }
    //iPhone都支持硬解，只是性能不同。交给用户选择
    //    _canHWAccel = false;
}

int VideoDecoder::videotoolbox_retrieve_data(AVCodecContext *s, AVFrame *frame) {
#if CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID
    return 0;
#else
    if (!_canHWAccel) {
        return 0;
    }
    CVPixelBufferRef pixbuf = (CVPixelBufferRef)frame->data[3];
    OSType pixel_format = CVPixelBufferGetPixelFormatType(pixbuf);
    CVReturn err;
    int planes, i;
    char codec_str[32];
    
    switch (pixel_format) {
        case kCVPixelFormatType_420YpCbCr8Planar: frame->format = AV_PIX_FMT_YUV420P; break;
        case kCVPixelFormatType_422YpCbCr8:       frame->format = AV_PIX_FMT_UYVY422; break;
        case kCVPixelFormatType_32BGRA:           frame->format = AV_PIX_FMT_BGRA; break;
#ifdef kCFCoreFoundationVersionNumber10_7
        case kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange: frame->format = AV_PIX_FMT_NV12; break;
#endif
        default:
            av_get_codec_tag_string(codec_str, sizeof(codec_str), s->codec_tag);
            av_log(NULL, AV_LOG_ERROR,
                   "%s: Unsupported pixel format\n", codec_str);
            return AVERROR(ENOSYS);
    }
    
    err = CVPixelBufferLockBaseAddress(pixbuf, kCVPixelBufferLock_ReadOnly);
    if (err != kCVReturnSuccess) {
        av_log(NULL, AV_LOG_ERROR, "Error locking the pixel buffer.\n");
        return AVERROR_UNKNOWN;
    }
    
    if (CVPixelBufferIsPlanar(pixbuf)) {
        planes = CVPixelBufferGetPlaneCount(pixbuf);
        for (i = 0; i < planes; i++) {
            frame->data[i]     = reinterpret_cast<uint8_t *>(CVPixelBufferGetBaseAddressOfPlane(pixbuf, i));
            frame->linesize[i] = CVPixelBufferGetBytesPerRowOfPlane(pixbuf, i);
        }
    } else {
        frame->data[0] = reinterpret_cast<uint8_t *>(CVPixelBufferGetBaseAddress(pixbuf));
        frame->linesize[0] = CVPixelBufferGetBytesPerRow(pixbuf);
    }
    
    CVPixelBufferUnlockBaseAddress(pixbuf, kCVPixelBufferLock_ReadOnly);
    
    return 0;
#endif
}

void VideoDecoder::videotoolbox_uninit(AVCodecContext *s) {
#if CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID
#else
    if (s) {
        av_videotoolbox_default_free(s);
    }
#endif
}

int VideoDecoder::videotoolbox_init(AVCodecContext *s)
{
#if CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID
    return 0;
#else
    int ret = av_videotoolbox_default_init(s);
    if (ret < 0) {
        LOGD("Create videotoolbox hwaccel failed");
        videotoolbox_uninit(s);
    }
    
    return ret;
#endif
}

enum AVPixelFormat VideoDecoder::get_format(AVCodecContext *s, const enum AVPixelFormat *pix_fmts) {
    const enum AVPixelFormat *p;
    int ret;
    
    for (p = pix_fmts; *p != AV_PIX_FMT_NONE; p++) {
        const AVPixFmtDescriptor *desc = av_pix_fmt_desc_get(*p);
        
        if (!(desc->flags & AV_PIX_FMT_FLAG_HWACCEL))
            break;
        
        if (*p == AV_PIX_FMT_VIDEOTOOLBOX) {
            if (videotoolbox_init(s) < 0) {
                return AV_PIX_FMT_NONE;
            }
        }
        break;
    }
    
    return *p;
}

int VideoDecoder::open(AVCodecContext *videoCtx, enum AVCodecID codecId) {
    _srcVideoCtx = videoCtx;
    _srcCodecId = codecId;
    _curDts = AV_NOPTS_VALUE;
    _videoNbFrames = 0;
    
    _videoCtx = nullptr;
    AVCodec *codec = nullptr;
    if (_curCodecName.empty()) {
        codec = avcodec_find_decoder(codecId);
    } else {
        codec = avcodec_find_decoder_by_name(_curCodecName.c_str());
    }
    
    int ret = 0;
    if(!codec) {
        LOGE("%s", "video codec is null!\n");
        ret = -1;
        goto fail;
    }
    LOGD("video codec name: %s\n", codec->name);
    LOGD("videoCodec->width: %d, videoCodec->height: %d\n", videoCtx->width, videoCtx->height);
    _videoCtx = avcodec_alloc_context3(codec);
    if(avcodec_copy_context(_videoCtx, videoCtx) != 0) {
        LOGE("%s", "Couldn't copy codec context");
        ret = -2;
        goto fail;
    }
    LOGD("_videoCtx->width: %d, _videoCtx->height: %d\n", _videoCtx->width, _videoCtx->height);
    //记录正确的画面宽和高
    //在android手机上后面的 avcodec_open2 会将_videoCtx->width和_videoCtx->height改成
    //错误的值，在getVideo里的avcodec_decode_video2之后又会改成正确的值，详见打出的log
    _dstFrameWidth = _videoCtx->width;
    _dstFrameHeight = _videoCtx->height;
    _fps = _videoCtx->framerate.num / _videoCtx->framerate.den;
    
#if CC_TARGET_PLATFORM != CC_PLATFORM_ANDROID
    //编译时需要打开 --enable-pthreads 选项
    //将 _videoCtx->thread_count = 0
    //这样ffmpeg会根据设备的cpu数目加1 决定 thread_count, 并且开这么多个线程解码
    if (!_canHWAccel) {
        //硬解则不开多线程，warning 说不稳定
        _videoCtx->thread_count = 0;
    } else {
        _videoCtx->get_format = get_format;
    }
#else
    _videoCtx->thread_count = 0;
#endif
    //refcounted_frames默认为零，av_decode_video2在解下一帧时上一帧就不可用了，因此我们必须通过sws_scale或memcpy拷出去
    //但sws_scale 或 memcpy 很浪费时间，所以我们自己控制释放就不用多一次拷贝了
    //该值非零即表示由caller来控制释放 av_decode_video2 解出的Frame，这样就不用 sws_scale 或 memcpy 浪费时间了
    _videoCtx->refcounted_frames = 1;
    
    if((ret = avcodec_open2(_videoCtx, NULL, NULL)) < 0) {
        char ac[1024];
        av_strerror(ret, ac, 1024);
        LOGE("%s, error: %d, str: %s\n", "Unsupported codec!", ret, ac);
        ret = -3;
        goto fail;
    }
    
    LOGD("VideoDecoder::open: _videoCtx->width: %d, _videoCtx->height: %d\n, _videoCtx->coded_width: %d, _videoCtx->coded_height: %d\n", _videoCtx->width, _videoCtx->height, _videoCtx->coded_width, _videoCtx->coded_height);
    
    return ret;
    
fail:
    if (_videoCtx) {
        avcodec_free_context(&_videoCtx);
    }
    return ret;
}

int VideoDecoder::getCodec() {
    if (_isSupportAvcHWCodec || _isSupportHevcHWCodec || _canHWAccel) {
        return 1;
    }
    return 0;
}

void VideoDecoder::setCodec(int codec) {
    if (codec == getCodec()) {
        return;
    }
    switch (codec) {
        case 0:
            _isSupportHevcHWCodec = false;
            _isSupportAvcHWCodec = false;
#if CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID
            _curCodecName = "";
#endif
            _canHWAccel = false;
            break;
        case 1:
            _isSupportHevcHWCodec = true;
            _isSupportAvcHWCodec = true;
#if CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID
            _curCodecName = "h264_mediacodec";
#endif
            _canHWAccel = true;
            break;
        default:
            break;
    }
}

int VideoDecoder::getVideo(AVPixelFormat *pixFmt, int64_t *pts, AVFrame *frame, uint8_t *data[4], int linesize[4]) {
    
    AVPacket *packet = nullptr;
    _packetReader->consumeVideoPacket(&packet);
    if (nullptr == packet) {
        LOGD("getVideo: null packet");
        return 0;
    }
    
    //多线程是ffmpeg实现的，编译时打开 --enable-pthreads 选项，在 avcodec_open2 之前将 codecCtx->thread_count = 0
    //这样ffmpeg会根据设备的cpu数目加1 决定 thread_count, 并且开这么多个线程解码
    int gotFrame = 0;
    int size = avcodec_decode_video2(_videoCtx, frame, &gotFrame, packet);
    int64_t curDts = packet->dts;
    av_packet_free(&packet);
    
    if (size < 0) {
        LOGE("avcodec_decode_video2 error!");
        _player->setLastError(Player::Error::DECODE_ERROR);
        return -1;
    }
    if (!gotFrame) {
        LOGD("getVideo: not gotFrame");
        return 0;
    }
#if CC_TARGET_PLATFORM != CC_PLATFORM_ANDROID
    videotoolbox_retrieve_data(_videoCtx, frame);
#endif
    
    _curDts = curDts;
    //frame is referenced by ffmpeg, clone it!!! else ffmpeg will change it unexpectedly!!!
    *pts = _videoNbFrames;
    if (frame->repeat_pict > 0) {
        _videoNbFrames += frame->repeat_pict;
    } else {
        _videoNbFrames++;
    }

    *pixFmt = static_cast<AVPixelFormat>(frame->format);
    
    static int count = 0;
    static time_t ts = time(nullptr);
    if (time(nullptr) != ts) {
        LOGV("frameRate: %d", count);
        LOGD("VideoDecoder::getVideo: _videoCtx->width: %d, _videoCtx->height: %d\n, _videoCtx->coded_width: %d, _videoCtx->coded_height: %d, frame->linesize[0]: %d \n", _videoCtx->width, _videoCtx->height, _videoCtx->coded_width, _videoCtx->coded_height, frame->linesize[0]);
        
        count = 0;
        ts = time(nullptr);
    } else {
        count++;
    }
    
//    uint8_t *data[4] = {nullptr};
//    int linesize[4] = {0};
//    
//    int ret = av_image_alloc(data, linesize, frame->linesize[0], frame->height, *pixFmt, 1);
//    av_image_copy(data, linesize, (const uint8_t **)frame->data, frame->linesize, *pixFmt, frame->linesize[0], frame->height);
//
    if (frame->linesize[0] != _videoCtx->width) {
        if (data[0] == nullptr) {
            av_image_alloc(data, linesize, _videoCtx->width, _videoCtx->height, *pixFmt, 1);
        }
        if (frame->linesize[0] < _videoCtx->width) {
            _swsCtx = getSwsContext(*pixFmt,
                                    frame->linesize[0],
                                    frame->height,
                                    *pixFmt);
            size = sws_scale(_swsCtx, frame->data, frame->linesize, 0, frame->height, data, linesize);
            if (size < 0) {
                return 0;
            }
        } else {
            if (*pixFmt == AV_PIX_FMT_NV12 || *pixFmt == AV_PIX_FMT_NV21) {
                cropImg(frame->data[0], data[0], frame->linesize[0], frame->height, linesize[0], _videoCtx->height);
                cropImg(frame->data[1], data[1], frame->linesize[1], frame->height/2, linesize[1], _videoCtx->height/2);
            } else {
                cropImg(frame->data[0], data[0], frame->linesize[0], frame->height, _videoCtx->width, _videoCtx->height);
                cropImg(frame->data[1], data[1], frame->linesize[1], frame->height/2, linesize[1], _videoCtx->height/2);
                cropImg(frame->data[2], data[2], frame->linesize[2], frame->height/2, linesize[2], _videoCtx->height/2);
            }
        }
        return 2;
    }
//    frame->linesize[0] = _videoCtx->width;
//    frame->linesize[1] = _videoCtx->width;
//    size = sws_scale(_swsCtx, data, linesize, 0, frame->height, frame->data, frame->linesize);
//    av_freep(&data[0]);
//    if (size < 0) {
//        return 0;
//    }
    
//    if (*pixFmt == AV_PIX_FMT_NV12 || *pixFmt == AV_PIX_FMT_NV21) {
//        if (frame->linesize[0] > _videoCtx->width) {
//            cropImg(frame->data[0], frame->linesize[0], frame->height, _videoCtx->width);
//        }
//        if (frame->linesize[1] > _videoCtx->width) {
//            cropImg(frame->data[1], frame->linesize[1], frame->height/2, _videoCtx->width);
//        }
//    }
    
    return 1;
}
    
void VideoDecoder::cropImg(const uint8_t *const src, uint8_t *const dst, int wSrc, int hSrc, int wDst, int hDst) {
    
    const uint8_t *pSrc = src;
    uint8_t *pDst = dst;
    int h = min(hSrc, hDst);
    int gap = (wSrc - wDst);
    for (int i = 0; i < h; i++) {
        for (int j = 0; j < wDst; j++) {
            *pDst = *pSrc;
            pSrc++;
            pDst++;
        }
        pSrc += gap;
    }
}
    
void VideoDecoder::cropImg(uint8_t *src, int wSrc, int hSrc, int wDst) {
    uint8_t *pDst = src + wDst;
    for (int i = 1; i < hSrc; i++) {
        uint8_t *pSrc = src + i * wSrc;
        for (int j = 0; j < wDst; j++) {
            *pDst = pSrc[j];
            pDst++;
        }
    }
}
    
struct SwsContext * VideoDecoder::getSwsContext(AVPixelFormat srcPixFmt, int width, int height, AVPixelFormat dstPixFmt) {
    if (srcPixFmt == _srcPixFmt &&
        width == _srcFrameWidth &&
        height == _srcFrameHeight &&
        dstPixFmt == _dstPixFmt) {
        return _swsCtx;
    }
    _srcPixFmt = srcPixFmt;
    switch (_srcPixFmt) {
        case AV_PIX_FMT_YUVJ420P:
            _srcPixFmt = AV_PIX_FMT_YUV420P;
            break;
        case AV_PIX_FMT_YUVJ422P:
            _srcPixFmt = AV_PIX_FMT_YUV422P;
            break;
        case AV_PIX_FMT_YUVJ444P:
            _srcPixFmt = AV_PIX_FMT_YUV444P;
            break;
        case AV_PIX_FMT_YUVJ440P:
            _srcPixFmt = AV_PIX_FMT_YUV440P;
        default:
            break;
    }
    
    _srcFrameWidth = width;
    _srcFrameHeight = height;
    _dstPixFmt = dstPixFmt;
    LOGD("pixFormat: %d, \
         AV_PIX_FMT_ARGB: %d, \
         AV_PIX_FMT_YUV420P: %d, \
         srcFrameWidth: %d, \
         srcFrameHeight: %d, \
         dstFrameWidth: %d, \
         dstFrameHeight: %d",
         _srcPixFmt,
         AV_PIX_FMT_ARGB,
         AV_PIX_FMT_YUV420P,
         _srcFrameWidth,
         _srcFrameHeight,
         _videoCtx->width,
         _videoCtx->height);
    
    safeFreeSwsContext();
    _swsCtx = sws_getContext(_srcFrameWidth,
                             _srcFrameHeight,
                             _srcPixFmt,
                             _videoCtx->width,
                             _videoCtx->height,
                             _dstPixFmt,
                             SWS_FAST_BILINEAR,
                             NULL,
                             NULL,
                             NULL);
    return _swsCtx;
}

}
