#ifndef __SL_H__
#define __SL_H__

#include "platform/CCPlatformConfig.h"

#if CC_TARGET_PLATFORM == CC_PLATFORM_MAC
#include "platform/mac/CCGL-mac.h"
#elif CC_TARGET_PLATFORM == CC_PLATFORM_IOS
#include "platform/ios/AudioQueue.hpp"
#elif CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID
#include "platform/android/OpenSL.hpp"
#elif CC_TARGET_PLATFORM == CC_PLATFORM_WIN32
#include "platform/win32/CCGL-win32.h"
#elif CC_TARGET_PLATFORM == CC_PLATFORM_WP8 || CC_TARGET_PLATFORM == CC_PLATFORM_WINRT
#include "platform/winrt/CCGL.h"
#elif CC_TARGET_PLATFORM == CC_PLATFORM_LINUX
#include "platform/linux/CCGL-linux.h"
#endif

#endif   // __SL_H__
