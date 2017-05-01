//
//  FileUtil.cpp
//  vrplayer
//
//  Created by 单强 on 16/9/26.
//  Copyright © 2016年 Facebook. All rights reserved.
//

#include "FileUtil.hpp"
#include "Util.h"
#include <stdlib.h>
#import <Foundation/Foundation.h>

string FileUtil::getWritablePath() {
    // save to document folder
    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    NSString *documentsDirectory = [paths objectAtIndex:0];
    std::string strRet = [documentsDirectory UTF8String];
    strRet.append("/");
    return strRet;
}

string FileUtil::getData(const std::string& filename)
{
    string ret;
    if (filename.empty()) {
        return ret;
    }

    string fullPath = filename;

    if (fullPath[0] != '/') {
        fullPath = [[[NSBundle mainBundle] bundlePath] UTF8String];
        fullPath += "/" + filename;
    }
    
    do
    {
        const char* mode = nullptr;
        mode = "rb";

        FILE *fp = fopen(fullPath.c_str(), mode);
        if (!fp) {
            break;
        }

        long fileSize;
        fseek(fp,0,SEEK_END);
        fileSize = ftell(fp);
        fseek(fp,0,SEEK_SET);
        ret.resize(fileSize);
        fread(&ret[0], sizeof(unsigned char), fileSize, fp);
        fclose(fp);
    } while (0);

    return ret;
}
