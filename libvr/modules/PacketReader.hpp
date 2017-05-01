//
//  PacketReader.hpp
//  vrplayer
//
//  Created by 单强 on 2017/3/12.
//  Copyright © 2017年 Facebook. All rights reserved.
//

#ifndef PacketReader_hpp
#define PacketReader_hpp

#include "VideoFile.hpp"

namespace geeek {

    class Player;
    class Audio;
    class Video;
class PacketReader {
public:
    PacketReader(Player *player, VideoFile *videoFile, Audio *audio, Video *video);
    ~PacketReader();
    
    double getCacheProgress();
    int seek(double percent);
    void start();
    void stop();
    void consumeAudioPacket(AVPacket **packet);
    void consumeVideoPacket(AVPacket **packet);
    bool isEnd();
    
private:
    bool atLeast();
    void flushAudioPackets();
    void flushVideoPackets();
    
    void freePackets(queue<AVPacket*> *packets);
    void produceAudioPacket(AVPacket *packet);
    void produceVideoPacket(AVPacket *packet);
    
private:
    queue<AVPacket*> _audioPackets;
    queue<AVPacket*> _videoPackets;
    mutex _mutexAudioPackets;
    mutex _mutexVideoPackets;
    mutex _mutexCv;
    condition_variable _cv;
    
    volatile bool _isExitThread;
    volatile bool _isEnd;
    thread *_readerThread;
    int64_t _audioDts;
    int64_t _videoDts;
    Player *_player;
    VideoFile *_videoFile;
    Audio *_audio;
    Video *_video;
};
    
}

#endif /* PacketReader_hpp */
