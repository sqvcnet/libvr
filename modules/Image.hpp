//
//  Image.hpp
//  vrplayer
//
//  Created by 单强 on 16/9/26.
//  Copyright © 2016年 Facebook. All rights reserved.
//

#ifndef Image_hpp
#define Image_hpp

#include "std.h"

class Image {
public:
    Image();
    string decompressJpg(const string &compressedData);
    int getWidth();
    int getHeight();
    
private:
    int _width;
    int _height;
};

#endif /* Image_hpp */
