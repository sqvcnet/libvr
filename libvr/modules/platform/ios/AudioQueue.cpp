//
//  AudioQueue.cpp
//  vrplayer
//
//  Created by 单强 on 16/7/27.
//  Copyright © 2016年 Facebook. All rights reserved.
//

#include "AudioQueue.hpp"

AudioQueue::AudioQueue()
:_delegate(nullptr)
,_audioThread(nullptr)
,_isExitThread(true)
,_audioQueueRef(nullptr)
,_bufSize(4096)
,_outBuffers{nullptr}
{
    
}

void AudioQueue::interruptionListener(void *inClientData, UInt32 inInterruptionState) {
    return;
}

void AudioQueue::determineOutputDevice() {
    CFDictionaryRef dict = nullptr;
    UInt32 dataSize = sizeof(dict);
    OSStatus error = AudioSessionGetProperty(kAudioSessionProperty_AudioRouteDescription, &dataSize, (void*)(&dict));
    if (error != 0) {
        return;
    }
    CFStringRef tmp = CFStringCreateWithCString(CFAllocatorGetDefault(), "RouteDetailedDescription_Outputs", kCFStringEncodingUTF8);
    const CFArrayRef outputs = (const CFArrayRef)CFDictionaryGetValue(dict, tmp);
    CFRelease(tmp);
    CFIndex count = CFArrayGetCount(outputs);
    const CFStringRef *keys[256];
    const void *values[256];
    for (int i = 0; i < count; i++) {
        dict = reinterpret_cast<CFDictionaryRef>(CFArrayGetValueAtIndex(outputs, i));
        
        tmp = CFStringCreateWithCString(CFAllocatorGetDefault(), "RouteDetailedDescription_IsHeadphones", kCFStringEncodingUTF8);
        const CFBooleanRef isHeadphone = reinterpret_cast<const CFBooleanRef>(CFDictionaryGetValue(dict, tmp));
        CFRelease(tmp);
        if (CFBooleanGetValue(isHeadphone)) {
            UInt32 route = kAudioSessionOverrideAudioRoute_None;
            error = AudioSessionSetProperty(kAudioSessionProperty_OverrideAudioRoute, sizeof(route), &route);
            return;
        }
//        CFDictionaryGetKeysAndValues(dict, (const void **)keys, (const void **)values);
    }
    
    UInt32 route = kAudioSessionOverrideAudioRoute_Speaker;
    error = AudioSessionSetProperty(kAudioSessionProperty_OverrideAudioRoute, sizeof(route), &route);
}

void AudioQueue::propListener(void *inClientData, AudioSessionPropertyID inID, UInt32 inDataSize, const void *inData) {
    AudioQueue *self = reinterpret_cast<AudioQueue*>(inClientData);
    if (inID == kAudioSessionProperty_AudioRouteChange) {
        const CFDictionaryRef dict = reinterpret_cast<const CFDictionaryRef>(inData);
//        CFIndex count = CFDictionaryGetCount(dict);
//        const CFStringRef *keys[256];
//        const void *values[256];
//        CFDictionaryGetKeysAndValues(dict, (const void **)keys, (const void **)values);
        CFStringRef tmp = CFStringCreateWithCString(CFAllocatorGetDefault(), kAudioSession_AudioRouteChangeKey_Reason, kCFStringEncodingUTF8);
        const CFNumberRef reason = (const CFNumberRef)CFDictionaryGetValue(dict, tmp);
        CFRelease(tmp);
        int iReason = -1;
        CFNumberGetValue(reason, kCFNumberIntType, (void*)(&iReason));
//        if (iReason == kAudioSessionRouteChangeReason_NewDeviceAvailable) {
//            tmp = CFStringCreateWithCString(CFAllocatorGetDefault(), "OutputDeviceDidChange_NewRoute", kCFStringEncodingUTF8);
//            const CFStringRef newRoute = (const CFStringRef)CFDictionaryGetValue(dict, tmp);
//            CFRelease(tmp);
//            const char *cNewRoute = CFStringGetCStringPtr(newRoute, kCFStringEncodingUTF8);
//            if (strncmp(cNewRoute, "HeadsetInOut", strlen("HeadsetInOut")) == 0) {
//                UInt32 route = kAudioSessionOverrideAudioRoute_None;
//                AudioSessionSetProperty(kAudioSessionProperty_OverrideAudioRoute, sizeof(route), &route);
//            }
//        }
        if (iReason == kAudioSessionRouteChangeReason_NewDeviceAvailable ||
            iReason == kAudioSessionRouteChangeReason_OldDeviceUnavailable) {
            self->determineOutputDevice();
        }
    }
    return;
}

void AudioQueue::init(AudioOutputDelegate *delegate) {
    OSStatus error = 0;
//    if (!_delegate) {
        error = AudioSessionInitialize(NULL, NULL, interruptionListener, this);
    
        UInt32 category = kAudioSessionCategory_PlayAndRecord;
        error = AudioSessionSetProperty(kAudioSessionProperty_AudioCategory, sizeof(category), &category);
    
        determineOutputDevice();
//        UInt32 route = kAudioSessionOverrideAudioRoute_Speaker; //默认kAudioSessionOverrideAudioRoute_None，听筒
//        AudioSessionSetProperty(kAudioSessionProperty_OverrideAudioRoute, sizeof(route), &route);
        
        error = AudioSessionAddPropertyListener(kAudioSessionProperty_AudioRouteChange, propListener, this);
//
//        UInt32 inputAvailable = 0;
//        UInt32 size = sizeof(inputAvailable);
//        // we do not want to allow recording if input is not available
//        error = AudioSessionGetProperty(kAudioSessionProperty_AudioInputAvailable, &size, &inputAvailable);
//        
//        // we also need to listen to see if input availability changes
//        error = AudioSessionAddPropertyListener(kAudioSessionProperty_AudioInputAvailable, propListener, this);
//        
        error = AudioSessionSetActive(true);
//    }
    _delegate = delegate;
    return;
}

void AudioQueue::audioQueueCallback(void * outUserData, AudioQueueRef outAQ, AudioQueueBufferRef outBuffer) {
  AudioQueue *self = reinterpret_cast<AudioQueue *>(outUserData);
  self->play(outBuffer);
}

int AudioQueue::open(int channels, int sampleRate) {
    //确保重定向为Speaker,百度语音会重定向为听筒
//    UInt32 route = kAudioSessionOverrideAudioRoute_Speaker; //默认kAudioSessionOverrideAudioRoute_None，听筒
//    AudioSessionSetProperty(kAudioSessionProperty_OverrideAudioRoute, sizeof(route), &route);
//    determineOutputDevice();
    //Siri和百度语音会设置AudioSession的状态，所以需要再init重置为正确的状态，见VoiceRecognizerSiri.m和VoiceRecognizerBaidu.m
    init(_delegate);
  if (isOpened()) {
    LOGD("AudioQueue::open: already opened");
    return noErr;
  }
    _channels = channels;
    _sampleRate = sampleRate;
  
  // prepare the format description for our output
  AudioStreamBasicDescription streamDescription;
  streamDescription.mSampleRate = sampleRate;
  streamDescription.mFormatID = kAudioFormatLinearPCM;
  streamDescription.mFormatFlags = kLinearPCMFormatFlagIsSignedInteger;
  streamDescription.mFramesPerPacket = 1;
  streamDescription.mChannelsPerFrame = 2;
  streamDescription.mBitsPerChannel = 16;
  streamDescription.mBytesPerFrame = streamDescription.mBitsPerChannel * streamDescription.mChannelsPerFrame / 8;
  streamDescription.mBytesPerPacket = streamDescription.mBytesPerFrame * streamDescription.mFramesPerPacket;
  
  // init new output instance
  OSStatus error = AudioQueueNewOutput(&streamDescription,           // Format
                              audioQueueCallback,    // Unused Callback, which needs to be provided to have a proper instance
                              this,                        // User data, passed to the callback
                              NULL,                        // RunLoop
                              kCFRunLoopCommonModes,       // RunLoop mode
                              0,                           // Flags ; must be zero (per documentation)...
                              &_audioQueueRef);    // Output
  
    allocBuffers();
    for (int i = 0; i < BUFFER_NUM; i++) {
        play(_outBuffers[i]);
    }

//  float volume = 1.0;
//  error = AudioQueueSetParameter(_audioQueueRef, kAudioQueueParam_Volume, volume * volume * volume);
//
  // start queue
  error = AudioQueueStart(_audioQueueRef, NULL);
  LOGD("Starting AudioQueue (status = %d)", error);
  
  _isExitThread = false;

    return 0;
}

void AudioQueue::allocBuffers() {
    for (int i = 0; i < BUFFER_NUM; i++) {
        AudioQueueAllocateBuffer(_audioQueueRef, static_cast<UInt32>(_bufSize), &_outBuffers[i]);
        _outBuffers[i]->mAudioDataByteSize = static_cast<UInt32>(_bufSize);
        _outBuffers[i]->mUserData = this;
    }
}

void AudioQueue::freeBuffers() {
    for (int i = 0; i < BUFFER_NUM; i++) {
        if (_outBuffers[i]) {
            AudioQueueFreeBuffer(_audioQueueRef, _outBuffers[i]);
            _outBuffers[i] = nullptr;
        }
    }
}

bool AudioQueue::isOpened() {
    return !_isExitThread;
}

void AudioQueue::play(AudioQueueBufferRef outBuffer)
{
  _delegate->consumeFrames(1);
  
  OSStatus status;
  char *data = nullptr;
  size_t size = 0;
  if (_delegate->getFrame(&data, &size)) {
      if (size != _bufSize) {
          _bufSize = size;
//          close();
//          open(_channels, _sampleRate);
          freeBuffers();
          allocBuffers();
          memcpy(_outBuffers[0]->mAudioData, data, size);
          status = AudioQueueEnqueueBuffer(_audioQueueRef, _outBuffers[0], 0, NULL);
          for (int i = 1; i < BUFFER_NUM; i++) {
              play(_outBuffers[i]);
          }
          return;
      }
      memcpy(outBuffer->mAudioData, data, size);
  } else {
      memset(outBuffer->mAudioData, 0, size);
  }
  
  status = AudioQueueEnqueueBuffer(_audioQueueRef, outBuffer, 0, NULL);
}

void AudioQueue::close() {
    if (_audioQueueRef) {
        AudioQueueStop(_audioQueueRef, false);
        LOGD("audioqueue async stopped");
        freeBuffers();
        AudioQueueDispose(_audioQueueRef, false);
        LOGD("audioqueue async disposed");
        _audioQueueRef = nullptr;
    }
    _isExitThread = true;
    LOGD("audioqueue stopped and disposed");
    return;
}
