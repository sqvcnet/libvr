//
//  FileUtil.hpp
//  vrplayer
//
//  Created by 单强 on 16/9/26.
//  Copyright © 2016年 Facebook. All rights reserved.
//

#ifndef FileUtil_hpp
#define FileUtil_hpp

#include "std.h"

#if CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID
extern "C" {
#include "android/asset_manager.h"
}
#endif

class FileUtil {
public:
#if CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID
    static void setassetmanager(AAssetManager* a);
    static void setWritablePath(const char *path);
#endif
    static string getFullPath(const std::string& filename);
    static string getWritablePath();
    static string getData(const std::string& filename);
#if CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID
private:
    static AAssetManager* assetmanager;
    static string _writablePath;
#endif
};

#endif /* FileUtil_hpp */
