#include "audiodecodeloop.h"
namespace LQF {
AudioDecodeLoop::AudioDecodeLoop()
{

}

AudioDecodeLoop::~AudioDecodeLoop()
{
    Stop();
    if(aac_decoder_)
        delete aac_decoder_;
    if(pcm_buf_)
        delete [] pcm_buf_;
    if(audio_out_sdl_)
        delete audio_out_sdl_;
}

RET_CODE AudioDecodeLoop::Init(const Properties &properties)
{
    aac_decoder_ = new AACDecoder();
    if(!aac_decoder_)
    {
        LogError("new AACDecoder() failed");
        return RET_ERR_OUTOFMEMORY;
    }
    Properties properties2;
    if(aac_decoder_->Init(properties2) != RET_OK)
    {
        LogError("aac_decoder_ Init failed");
        return RET_FAIL;
    }
    pcm_buf_ = new uint8_t[PCM_BUF_MAX_SIZE];
    if(!pcm_buf_)
    {
        LogError("pcm_buf_ new failed");
        return RET_ERR_OUTOFMEMORY;
    }
    return RET_OK;
}

RET_CODE AudioDecodeLoop::Output(const uint8_t *pcm_buf, const uint32_t size)
{
    if(audio_out_sdl_)
        return audio_out_sdl_->Output(pcm_buf, size);
    else
        return RET_FAIL;
}

void AudioDecodeLoop::handle(int what, MsgBaseObj *data)
{
    if(what == RTMP_BODY_AUD_SPEC)
    {
        AudioSpecMsg *aud_spec = (AudioSpecMsg *)data;
        // 目前没有做音视频同步，所以现在这里进行音频输出的初始化
        if(!audio_out_sdl_)
        {
            // 初始化audio out相关
            audio_out_sdl_ = new AudioOutSDL();
            if(!audio_out_sdl_)
            {
                LogError("new AudioOutSDL() failed");
                return;
            }
            Properties aud_out_properties;
            aud_out_properties.SetProperty("sample_rate",aud_spec->sample_rate_);
            aud_out_properties.SetProperty("channels", aud_spec->channels_);
            if(audio_out_sdl_->Init(aud_out_properties) != RET_OK)
            {
                LogError("audio_out_sdl Init failed");
                delete audio_out_sdl_;
                audio_out_sdl_ = NULL;
            }
        }

        delete aud_spec;
    }
    else if(what == RTMP_BODY_AUD_RAW)
    {
        AudioRawMsg *aud_raw = (AudioRawMsg *)data;
        pcm_buf_size_ = PCM_BUF_MAX_SIZE;
        // 可以发送adts header, 如果不发送adts则要初始化 ctx的参数
        if(aac_decoder_->Decode(aud_raw->data , aud_raw->size ,
                                pcm_buf_, pcm_buf_size_) == RET_OK)
        {
            if(_callableObject)
                _callableObject(pcm_buf_, pcm_buf_size_);
        }
        delete aud_raw;     // 要手动释放资源
    }
    else
    {
        LogError("can't handle what:%d", what);
        delete data;
    }
}
}
