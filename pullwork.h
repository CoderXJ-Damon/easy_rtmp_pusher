#ifndef PULLWORK_H
#define PULLWORK_H
#include "mediabase.h"
#include "audiodecodeloop.h"
#include "videodecodeloop.h"
#include "audiooutsdl.h"
#include "videooutputloop.h"
#include "videooutsdl.h"
#include "rtmpplayer.h"
#include "avsync.h"
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
    void pcmCallback(uint8_t* pcm, uint32_t size, int64_t pts);
    void displayVideo(uint8_t* yuv, uint32_t size, int32_t format);
    // 数据回调
    void yuvCallback(uint8_t* yuv, uint32_t size, int64_t pts);

    int avSyncCallback(int64_t pts, int32_t duration);

private:
    std::string rtmp_url_;

    RTMPPlayer *rtmp_player_ = NULL;
    AudioDecodeLoop *audio_decode_loop_ = NULL; //音频解码线程
    AudioOutSDL *audio_out_sdl_ = NULL;

    VideoDecodeLoop *video_decode_loop_; //视频解码线程
    VideoOutputLoop *video_output_loop_ = NULL;
    VideoOutSDL *video_out_sdl_ = NULL;
    AVSync *avsync_ = NULL;
};
}
#endif // PULLWORK_H
