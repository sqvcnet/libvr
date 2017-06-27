//
//  std.h
//  vrplayer
//
//  Created by 单强 on 16/7/27.
//  Copyright © 2016年 Facebook. All rights reserved.
//

#ifndef std_h
#define std_h

#include <map>
#include <thread>
#include <string>
#include <queue>
#include <list>
#include <mutex>
#include <condition_variable>
using namespace std;


extern "C" {
//#include "ffmpeg.h"
    #include "libavutil/avutil.h"
    #include "libavutil/imgutils.h"
    #include "libavutil/opt.h"
    #include "libavutil/time.h"
    #include "libavformat/avformat.h"
    #include "libavcodec/avcodec.h"
    #include "libswscale/swscale.h"
    #include "libswresample/swresample.h"
        
//    #include "curl.h"
}

#include "Util.h"

class SyncDelegate {
public:
    virtual double computeAudioClock(int64_t nbSamples) = 0;
    virtual double computeVideoClock(int64_t nbFrames) = 0;
};

#endif /* std_h */
