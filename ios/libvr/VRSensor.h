//
//  VRSensor.h
//  libvr
//
//  Created by 单强 on 2017/6/20.
//  Copyright © 2017年 单强. All rights reserved.
//

#import <GLKit/GLKit.h>

@interface VRSensor : NSObject

@property (nonatomic, assign, readonly, getter=isReady) BOOL ready;

+ (instancetype)inst;

- (void)start;
- (void)stop;

- (GLKMatrix4)modelView;

@end
