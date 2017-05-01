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
#include "android/asset_manager.h"
#include "android/asset_manager_jni.h"

AAssetManager* FileUtil::assetmanager = nullptr;
string FileUtil::_writablePath;

void FileUtil::setassetmanager(AAssetManager* a) {
    if (nullptr == a) {
        LOGD("setassetmanager : received unexpected nullptr parameter");
        return;
    }

    FileUtil::assetmanager = a;
}

void FileUtil::setWritablePath(const char *path) {
    _writablePath = path;
}

string FileUtil::getData(const std::string& filename)
{
    string ret;
    if (filename.empty())
    {
        return ret;
    }

    string fullPath = filename;

    if (fullPath[0] != '/')
    {
        string relativePath = string();

        size_t position = fullPath.find("assets/");
        if (0 == position) {
            // "assets/" is at the beginning of the path and we don't want it
            relativePath += fullPath.substr(strlen("assets/"));
        } else {
            relativePath += fullPath;
        }
        LOGV("relative path = %s", relativePath.c_str());

        if (nullptr == FileUtil::assetmanager) {
            LOGD("... FileUtilsAndroid::assetmanager is nullptr");
            return ret;
        }

        // read asset data
        AAsset* asset =
            AAssetManager_open(FileUtil::assetmanager,
                               relativePath.c_str(),
                               AASSET_MODE_UNKNOWN);
        if (nullptr == asset) {
            LOGD("asset is nullptr");
            return ret;
        }

        size_t fileSize = AAsset_getLength(asset);
        ret.resize(fileSize);
        AAsset_read(asset, (void*)&ret[0], fileSize);
        AAsset_close(asset);
    }
    else
    {
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
    }

    return ret;
}
