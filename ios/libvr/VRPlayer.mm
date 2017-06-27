//
//  RNTVRPlayer.mm
//  RNVRPlayer
//
//  Created by 单强 on 2017/6/20.
//  Copyright © 2017年 单强. All rights reserved.
//

#import "VRPlayer.h"
#import "VRSensor.h"
#include "Player.hpp"
#include "Renderer.h"

@implementation VRPlayer {
    geeek::Player *_player;
    int _mode;
    NSThread *_renderThread;
    CADisplayLink *_displayLink;
}

- (instancetype)init
{
    _player = new geeek::Player();
    
    EAGLContext *eaglContext = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES3];
    if (eaglContext == nil) {
        eaglContext = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2];
    }
    [EAGLContext setCurrentContext:eaglContext];
    
    self = [self initWithFrame:CGRectZero];

    self.context = eaglContext;
    self.delegate = self;

    if ([[UIDevice currentDevice] orientation] == UIDeviceOrientationLandscapeLeft) {
        _player->getRenderer()->setLandscape(true);
    } else {
        _player->getRenderer()->setLandscape(false);
    }
    [[NSNotificationCenter defaultCenter]
     addObserver:self selector:@selector(orientationChanged:)
     name:UIDeviceOrientationDidChangeNotification
     object:[UIDevice currentDevice]];
    
    return self;
}

- (void) orientationChanged:(NSNotification *)note
{
    UIDevice * device = note.object;
    switch(device.orientation)
    {
        case UIDeviceOrientationLandscapeLeft:
            _player->getRenderer()->setLandscape(true);
            break;
        case UIDeviceOrientationLandscapeRight:
            _player->getRenderer()->setLandscape(false);
            break;
        default:
            break;
    };
}

- (void)dealloc
{
    if (_player) {
        delete _player;
        _player = nullptr;
    }
}

- (void)threadMain {
    [_displayLink addToRunLoop:[NSRunLoop currentRunLoop] forMode:NSRunLoopCommonModes];
    CFRunLoopRun();
}

- (void)open:(NSString *)uri callback:(CallbackBlock)callback {
    if (!uri || [uri isEqualToString:@""]) {
        return;
    }

    _displayLink = [CADisplayLink displayLinkWithTarget:self selector:@selector(render:)];
//    [_displayLink addToRunLoop:[NSRunLoop currentRunLoop] forMode:NSRunLoopCommonModes];
    _renderThread = [[NSThread alloc] initWithTarget:self
                                            selector:@selector(threadMain)
                                              object:nil];
    [_renderThread start];

    _player->open([uri UTF8String], [=](const string &err) -> void {
        callback([NSString stringWithUTF8String:err.c_str()]);
    });
}

- (NSDictionary *)getProgress {
    NSDictionary *event = @{
                            @"progress": [[NSNumber alloc] initWithFloat:_player->getProgress()],
                            @"cacheProgress": [[NSNumber alloc] initWithFloat:_player->getCacheProgress()],
                            @"totalTime": [[NSNumber alloc] initWithFloat:_player->getTotalTime()],
                            @"error": [[NSNumber alloc] initWithInt:static_cast<int>(_player->getLastError())]
                            };
    return event;
}

- (void)play:(BOOL)isPlay {
    if (nullptr == _player) {
        NSException *e = [NSException
                          exceptionWithName:@"please open firstly"
                          reason:@"_player is nullptr"
                          userInfo:nil];
        @throw e;
        return;
    }
    if (isPlay) {
        [self startSensorRenderer];
        _player->play();
    } else {
        _player->pause();
        [self saveCost];
    }
}

- (void)setCodec:(int)codec {
    _player->setCodec(codec);
}

- (void)close {
    _displayLink.paused = false;
    _player->close();

    [self stopSensorRenderer];
    //这里必须invalidate，否则_displayLink 隐式hold住self导致self不能被gc, dealloc的断点不会进入，_displayLink和self都不会被释放！！！
    [_displayLink invalidate];
    [_renderThread cancel];
}

- (void)seek:(double)seek {
    _player->seek(seek);
}

- (void)startSensorRenderer {
    [[VRSensor inst] start];
    _displayLink.paused = false;
}

- (void)stopSensorRenderer {
    [[VRSensor inst] stop];
    _displayLink.paused = true;
}

- (void)saveCost {
    Renderer::Mode tmp = static_cast<Renderer::Mode>(_mode);
    switch (tmp) {
        case Renderer::Mode::MODE_3D:
            [self stopSensorRenderer];
            break;
        case Renderer::Mode::MODE_360:
            [self startSensorRenderer];
            break;
        case Renderer::Mode::MODE_360_UP_DOWN:
            [self startSensorRenderer];
            break;
        case Renderer::Mode::MODE_3D_LEFT_RIGHT:
            [self stopSensorRenderer];
            break;
        case Renderer::Mode::MODE_360_SINGLE:
            [self startSensorRenderer];
            break;
        default:
            [self startSensorRenderer];
            break;
    }
}

- (void)setMode:(int)mode {
    if (mode != -1) {
        _mode = mode;
    }
    Renderer::Mode tmp = static_cast<Renderer::Mode>(_mode);
    _player->getRenderer()->setMode(tmp);
    switch (tmp) {
        case Renderer::Mode::MODE_3D:
            [[VRSensor inst] stop];
            break;
        case Renderer::Mode::MODE_360:
            [[VRSensor inst] start];
            break;
        case Renderer::Mode::MODE_360_UP_DOWN:
            [[VRSensor inst] start];
            break;
        case Renderer::Mode::MODE_3D_LEFT_RIGHT:
            [[VRSensor inst] stop];
            break;
        case Renderer::Mode::MODE_360_SINGLE:
            [[VRSensor inst] start];
            break;
        default:
            [[VRSensor inst] start];
            break;
    }
}

- (void)setRotateDegree:(int)degree {
    _player->getRenderer()->setRotateDegree(degree);
}

- (void)setViewPortDegree:(int)degree {
    _player->getRenderer()->setViewPortDegree(degree);
}

- (void)initRenderer:(int)width height:(int)height {
    _player->getRenderer()->init(width, height);
}

- (void)render:(CADisplayLink*)displayLink {
    [self display];
}

- (void)glkView:(GLKView *)view drawInRect:(CGRect)rect
{
    GLKMatrix4 head_from_start_matrix = [[VRSensor inst] modelView];
    _player->getRenderer()->render(head_from_start_matrix.m);
}

@end
