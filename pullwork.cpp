#include "pullwork.h"
#include "timeutil.h"
#include "avtimebase.h"
namespace LQF
{
PullWork::PullWork()
{

}

PullWork::~PullWork()
{
    if(rtmp_player_)
        delete rtmp_player_;
    if(audio_decode_loop_)
        delete audio_decode_loop_;
    if(video_decode_loop_)
        delete video_decode_loop_;
    if(video_output_loop_)
        delete video_output_loop_;
    if(avsync_)
        delete avsync_;
}

RET_CODE PullWork::Init(const Properties &properties)
{
    // rtmp拉流
    rtmp_url_ = properties.GetProperty("rtmp_url", "");
    if(rtmp_url_ == "")
    {
        LogError("rtmp url is null");
        return RET_FAIL;
    }
    avsync_ = new AVSync();
    if(avsync_->Init(AV_SYNC_AUDIO_MASTER) != RET_OK)
    {
        LogError("AVSync Init failed");
        return RET_FAIL;
    }
    audio_decode_loop_ = new AudioDecodeLoop();

    audio_decode_loop_->addCallback(std::bind(&PullWork::pcmCallback, this,
                                              std::placeholders::_1,
                                              std::placeholders::_2,
                                              std::placeholders::_3));
    if(!audio_decode_loop_)
    {
        LogError("new AudioDecodeLoop() failed");
        return RET_FAIL;
    }
    Properties aud_loop_properties;
    if(audio_decode_loop_->Init(aud_loop_properties)!= RET_OK)
    {
        LogError("audio_decode_loop_ Init failed");
        return RET_FAIL;
    }

    video_output_loop_ = new VideoOutputLoop();
    Properties video_out_loop_properties;
    if(video_output_loop_->Init(video_out_loop_properties)!= RET_OK)
    {
        LogError("video_output_loop_ Init failed");
        return RET_FAIL;
    }
    video_output_loop_->AddAVSyncCallback(std::bind(&PullWork::avSyncCallback, this,
                                                  std::placeholders::_1,
                                                  std::placeholders::_2));
    video_output_loop_->AddDisplayCallback(std::bind(&PullWork::displayVideo, this,
                                                   std::placeholders::_1,
                                                   std::placeholders::_2,
                                                   std::placeholders::_3));
    if(video_output_loop_->Start() != RET_OK) {
        LogError("video_output_loop_ Start   failed");
        return RET_FAIL;
    }

    video_decode_loop_ = new VideoDecodeLoop();

    video_decode_loop_->AddCallback(std::bind(&PullWork::yuvCallback, this,
                                              std::placeholders::_1,
                                              std::placeholders::_2,
                                              std::placeholders::_3));
    if(!video_decode_loop_)
    {
        LogError("new VideoDecodeLoop() failed");
        return RET_FAIL;
    }
    Properties vid_loop_properties;
    if(video_decode_loop_->Init(vid_loop_properties)!= RET_OK)
    {
        LogError("audio_decode_loop_ Init failed");
        return RET_FAIL;
    }

    AVPlayTime::GetInstance()->Rest();

    rtmp_player_ = new RTMPPlayer();
    if(!rtmp_player_)
    {
        LogError("new RTMPPlayer() failed");
        return RET_FAIL;
    }

    rtmp_player_->AddAudioCallback(std::bind(&PullWork::audioCallback, this,
                                             std::placeholders::_1,
                                             std::placeholders::_2,
                                             std::placeholders::_3));
    rtmp_player_->AddVideoCallback(std::bind(&PullWork::videoCallback, this,
                                             std::placeholders::_1,
                                             std::placeholders::_2,
                                             std::placeholders::_3));

    if(!rtmp_player_->Connect(rtmp_url_.c_str()))
    {
        LogError("rtmp_pusher connect() failed");
        return RET_FAIL;
    }
    rtmp_player_->Start();
    return RET_OK;
}

void PullWork::audioCallback(int what, MsgBaseObj *data, bool flush)
{
    int64_t cur_time = TimesUtil::GetTimeMillisecond();
    if(what == RTMP_BODY_AUD_SPEC)
    {

        if(!audio_out_sdl_)
        {
            // 初始化audio out相关
            audio_out_sdl_ = new AudioOutSDL(avsync_);
            if(!audio_out_sdl_)
            {
                LogError("new AudioOutSDL() failed");
                return ;
            }
            AudioSpecMsg *audio_spec = (AudioSpecMsg *)data;
            Properties aud_out_properties;
            aud_out_properties.SetProperty("sample_rate", audio_spec->sample_rate_);
            aud_out_properties.SetProperty("channels", audio_spec->channels_);
            delete audio_spec;
            if(audio_out_sdl_->Init(aud_out_properties) != RET_OK)
            {
                LogError("audio_out_sdl Init failed");
                return ;
            }
            // 初始化非常耗时，所以需要提前初始化好 有耗时到1秒
            LogInfo("%s:audio_out_init:t:%lld",AVPlayTime::GetInstance()->getKeyTimeTag(),
                    TimesUtil::GetTimeMillisecond() - cur_time);
        }
    }
    else if(what == RTMP_BODY_AUD_RAW)
    {
        audio_decode_loop_->Post(what, data, flush);
    }
    else
    {
        LogError("can't handle what:%d", what);
        delete data;
    }

    int64_t diff = TimesUtil::GetTimeMillisecond() - cur_time;
    if(diff>5)
        LogDebug("audioCallback t:%ld", diff);
}

void PullWork::videoCallback(int what, MsgBaseObj *data, bool flush)
{
//    return;
    int64_t cur_time = TimesUtil::GetTimeMillisecond();

    if(what == RTMP_BODY_METADATA) {
        if(!video_out_sdl_)
        {
            video_out_sdl_ = new VideoOutSDL();
            if(!video_out_sdl_)
            {
                LogError("new VideoOutSDL() failed");
                return;
            }
            Properties vid_out_properties;
            FLVMetadataMsg *metadata = (FLVMetadataMsg *)data;
            vid_out_properties.SetProperty("video_width", metadata->width);
            vid_out_properties.SetProperty("video_height",  metadata->height);
            vid_out_properties.SetProperty("win_x", 1000);
            vid_out_properties.SetProperty("win_title", "pull video display");
            delete metadata;
            if(video_out_sdl_->Init(vid_out_properties) != RET_OK)
            {
                LogError("video_out_sdl Init failed");
                return;
            }
            // 初始化非常耗时，所以需要提前初始化好 有耗时到1秒
            LogInfo("%s:video_out_init:t:%lld",AVPlayTime::GetInstance()->getKeyTimeTag(),
                    TimesUtil::GetTimeMillisecond() - cur_time);
        }
        return;
    }

    video_decode_loop_->Post(what, data, flush);

    int64_t diff = TimesUtil::GetTimeMillisecond() - cur_time;
    if(diff>5)
        LogInfo("videoCallback t:%ld", diff);
}

void PullWork::pcmCallback(uint8_t *pcm, uint32_t size, int64_t pts)
{
//    return;
//  LogInfo("pcm:%p, size:%d", pcm, pts);
    if(audio_out_sdl_)
        audio_out_sdl_->PushFrame(pcm, size, pts);
    else
        LogWarn("audio_out_sdl no open");
}

void PullWork::displayVideo(uint8_t *yuv, uint32_t size, int32_t format)
{
//    LogInfo("yuv:%p, size:%d", yuv, size);
    if(video_out_sdl_)
        video_out_sdl_->Output(yuv, size);
    else
        LogWarn("video_out_sdl no open");
}

void PullWork::yuvCallback(uint8_t *yuv, uint32_t size, int64_t pts)
{
//      LogInfo("yuv:%p, size:%d", yuv, size);
    if(video_output_loop_)
        video_output_loop_->PushFrame(yuv, size, pts);
    else
        LogWarn("video_out_sdl no open");
}

int PullWork::avSyncCallback(int64_t pts, int32_t duration)
{
    return avsync_->GetVideoSyncResult(pts, duration);
}
}
