//
//  VideoDecoder.hpp
//  vrplayer
//
//  Created by 单强 on 2017/3/12.
//  Copyright © 2017年 Facebook. All rights reserved.
//

#ifndef Video_Decoder_hpp
#define Video_Decoder_hpp

#include "std.h"
#include "Video.hpp"

namespace geeek {
    
class Player;
class PacketReader;
class VideoDecoder : public VideoDelegate {
private:
    static bool _isSupportHevcHWCodec;
    static bool _isSupportAvcHWCodec;
    
public:
    VideoDecoder(Player *player, PacketReader *packetReader);
    ~VideoDecoder();
    
    int64_t getCurDts();
    double getFps();
    int getWidth();
    int getHeight();
    void unsupportHWCodec();
    int getCodec();
    void setCodec(int codec);
    int open(AVCodecContext* videoCodec, enum AVCodecID codecId);
    void close();
    
    virtual int getVideo(AVPixelFormat *pixFmt, int64_t *pts, AVFrame *frame, uint8_t *data[4], int linesize[4]);
    
private:
    static enum AVPixelFormat get_format(AVCodecContext *s, const enum AVPixelFormat *pix_fmts);
    static int videotoolbox_init(AVCodecContext *s);
    static void videotoolbox_uninit(AVCodecContext *s);
    
    int videotoolbox_retrieve_data(AVCodecContext *s, AVFrame *frame);
    void cropImg(uint8_t *src, int wSrc, int hSrc, int wDst);
    void cropImg(const uint8_t *const src, uint8_t *const dst, int wSrc, int hSrc, int wDst, int hDst);
    struct SwsContext * getSwsContext(AVPixelFormat srcPixFmt, int width, int height, AVPixelFormat dstPixFmt);
    void safeFreeSwsContext();
    
private:
    AVFrame *_videoFrame;
    AVCodecContext *_videoCtx;
    struct SwsContext *_swsCtx;
    PacketReader *_packetReader;
    Player *_player;
    string _curCodecName;
    bool _canHWAccel;
    int64_t _curDts;
    int64_t _videoNbFrames;
    int _srcFrameWidth;
    int _srcFrameHeight;
    int _dstFrameWidth;
    int _dstFrameHeight;
    enum AVPixelFormat _srcPixFmt;
    enum AVPixelFormat _dstPixFmt;
};

}

#endif /* Video_Decoder_hpp */
