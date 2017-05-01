#ifndef Util_h
#define Util_h

#include <iostream>
#include <sstream>
#include <time.h>
#include <stdio.h>
#include <math.h>

#include "platform/CCPlatformConfig.h"
#if CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID

#include <android/log.h>
#define LOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, "libvr", __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG , "libvr", __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO  , "libvr", __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN  , "libvr", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR  , "libvr", __VA_ARGS__)

#else

#define LOGV(...) (printf(__VA_ARGS__),printf("\n"));
#define LOGD(...) (printf(__VA_ARGS__),printf("\n"));
#define LOGI(...) (printf(__VA_ARGS__),printf("\n"));
#define LOGW(...) (printf(__VA_ARGS__),printf("\n"));
#define LOGE(...) (printf(__VA_ARGS__),printf("\n"));

#endif // CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID

class Util {
public:
    static void xyz2r(float x, float y, float z, float &r, float &theta, float &phi);
    static void r2xyz(float r, float theta, float phi, float &x, float &y, float &z);
};

#endif /* Util_h */
