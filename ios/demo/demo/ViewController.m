//
//  ViewController.m
//  demo
//
//  Created by 单强 on 2017/6/27.
//  Copyright © 2017年 单强. All rights reserved.
//

#import "ViewController.h"
#import "VRPlayer.h"

@interface ViewController ()

@end

@implementation ViewController {
    VRPlayer *_vrplayer;
    NSTimer *_timer;
}

- (void)viewDidLoad {
    [super viewDidLoad];
    // Do any additional setup after loading the view, typically from a nib.
    _vrplayer = [[VRPlayer alloc] initWithFrame:self.view.frame];
    [self.view addSubview:_vrplayer];
    
    [_vrplayer open:@"./movies/GongKeJiDongDui3D.mp4" callback:^(NSString *err) {
        [_vrplayer play:YES];
        _timer = [NSTimer scheduledTimerWithTimeInterval:0.5
                                                  target:self
                                                selector:@selector(updateProgress)
                                                userInfo:nil
                                                 repeats:YES];
    }];
}

- (void)updateProgress {
    NSMutableDictionary *event = [NSMutableDictionary dictionaryWithDictionary:[_vrplayer getProgress]];
    NSNumber *progress = [event valueForKey:@"progress"];
    if ([progress doubleValue] >= 1.0) {
        [_timer invalidate];
        [_vrplayer close];
    }
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}


@end
