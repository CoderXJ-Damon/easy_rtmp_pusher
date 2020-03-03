#ifndef PULLWORK_H
#define PULLWORK_H
#include "mediabase.h"
#include "audiodecodeloop.h"
#include "videodecodeloop.h"
#include "audiooutsdl.h"
#include "videooutsdl.h"
#include "rtmpplayer.h"
namespace LQF
{
class PullWork
{
public:
    PullWork();
    ~PullWork();
    RET_CODE Init(const Properties& properties);

    // Audio编码后的数据回调
    void audioCallback(int what, MsgBaseObj *data, bool flush = false);
    // Video编码后的数据回调
    void videoCallback(int what, MsgBaseObj *data, bool flush = false);

    // pcm数据的数据回调
    void pcmCallback(uint8_t* pcm, uint32_t size);
    // 数据回调
    void yuvCallback(uint8_t* yuv, uint32_t size);

private:
    std::string rtmp_url_;

    RTMPPlayer *rtmp_player_ = NULL;
    AudioDecodeLoop *audio_decode_loop_ = NULL; //音频解码线程


    VideoDecodeLoop *video_decode_loop_; //视频解码线程

};
}
#endif // PULLWORK_H
