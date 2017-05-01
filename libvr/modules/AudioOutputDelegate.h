//
//  AudioOutputDelegate.h
//  vrplayer
//
//  Created by 单强 on 16/7/27.
//  Copyright © 2016年 Facebook. All rights reserved.
//

#ifndef AudioOutputDelegate_h
#define AudioOutputDelegate_h

#include "std.h"

class AudioOutputDelegate {
public:
    virtual void consumeFrames(int count) = 0;
    virtual bool getFrame(char **data, size_t *size) = 0;
};

#endif /* AudioOutputDelegate_h */
