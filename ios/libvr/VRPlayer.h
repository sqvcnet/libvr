//
//  VRPlayer.h
//  libvr
//
//  Created by 单强 on 2017/6/20.
//  Copyright © 2017年 单强. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <GLKit/GLKit.h>

@interface VRPlayer : GLKView<GLKViewDelegate>
- (instancetype)init;
typedef void(^CallbackBlock)(NSString *err);
- (void)open:(NSString *)uri callback:(CallbackBlock)callback;
- (void)seek:(double)seek;
- (void)play:(BOOL)isPlay;
- (void)close;
- (void)setMode:(int)mode;
- (void)setCodec:(int)codec;
- (void)setRotateDegree:(int)degree;
- (void)setViewPortDegree:(int)degree;
- (void)initRenderer:(int)width height:(int)height;
- (NSDictionary *)getProgress;
@end
