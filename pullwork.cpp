#include "pullwork.h"
#include "timeutil.h"
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



    audio_decode_loop_ = new AudioDecodeLoop();

    audio_decode_loop_->addCallback(std::bind(&PullWork::pcmCallback, this,
                                              std::placeholders::_1,
                                              std::placeholders::_2));
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


    video_decode_loop_ = new VideoDecodeLoop();

    video_decode_loop_->AddCallback(std::bind(&PullWork::yuvCallback, this,
                                              std::placeholders::_1,
                                              std::placeholders::_2));
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
//    if(what == RTMP_BODY_AUD_SPEC)
//    {
//        //        if(!audio_out_sdl)
//        //        {
//        //            // 初始化audio out相关
//        //            audio_out_sdl = new AudioOutSDL();
//        //            if(!audio_out_sdl)
//        //            {
//        //                LogError("new AudioOutSDL() failed");
//        //                return ;
//        //            }
//        //            AudioSpecMsg *audio_spec = (AudioSpecMsg *)data;
//        //            Properties aud_out_properties;
//        //            aud_out_properties.SetProperty("sample_rate", audio_spec->sample_rate_);
//        //            aud_out_properties.SetProperty("channels", audio_spec->channels_);
//        //            delete audio_spec;
//        //            if(audio_out_sdl->Init(aud_out_properties) != RET_OK)
//        //            {
//        //                LogError("audio_out_sdl Init failed");
//        //                return ;
//        //            }
//        //        }
//    }
//    else if(what == RTMP_BODY_AUD_RAW)
//    {
//        audio_decode_loop_->Post(what, data, flush);
//    }
//    else
//    {
//        LogError("can't handle what:%d", what);
//        delete data;
//    }
    audio_decode_loop_->Post(what, data, flush);
    int64_t diff = TimesUtil::GetTimeMillisecond() - cur_time;
    if(diff>5)
        LogInfo("audioCallback t:%ld", diff);
}

void PullWork::videoCallback(int what, MsgBaseObj *data, bool flush)
{
    int64_t cur_time = TimesUtil::GetTimeMillisecond();



    video_decode_loop_->Post(what, data, flush);

    int64_t diff = TimesUtil::GetTimeMillisecond() - cur_time;
    if(diff>5)
        LogInfo("videoCallback t:%ld", diff);
}

void PullWork::pcmCallback(uint8_t *pcm, uint32_t size)
{
    //    LogInfo("pcm:%p, size:%d", pcm, size);
    if(audio_decode_loop_)
        audio_decode_loop_->Output(pcm, size);
    else
        LogWarn("audio_out_sdl no open");
}

void PullWork::yuvCallback(uint8_t *yuv, uint32_t size)
{
    //    LogInfo("yuv:%p, size:%d", yuv, size);
    if(video_decode_loop_)
        video_decode_loop_->Output(yuv, size);
    else
        LogWarn("video_out_sdl no open");
}
}
