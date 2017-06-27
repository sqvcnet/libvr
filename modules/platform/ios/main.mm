//
//  main.m
//  libvr
//
//  Created by 单强 on 2017/5/1.
//  Copyright © 2017年 单强. All rights reserved.
//

#import <UIKit/UIKit.h>

#include "std.h"
#include "main.h"

@interface MainThread : NSObject
@end

@implementation MainThread

MainThread *mainThread = nil;
queue<pair<MainThreadFunc, void*> > queueMainThreadFunc;

void performInMainThread(MainThreadFunc func, void *param) {
    if (!mainThread) {
        mainThread = [[MainThread alloc] init];
    }
    queueMainThreadFunc.push(pair<MainThreadFunc, void*>(func, param));
    [mainThread performSelectorOnMainThread:@selector(inMainThread:) withObject:nil waitUntilDone:NO];
}

- (void)inMainThread:(NSObject *)obj
{
    while (!queueMainThreadFunc.empty()) {
        pair<MainThreadFunc, void*> funcParam = queueMainThreadFunc.front();
        funcParam.first(funcParam.second);
        queueMainThreadFunc.pop();
    }
}

@end
