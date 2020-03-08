#include "audiodecodeloop.h"
#include "avtimebase.h"
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
//    if(audio_out_sdl_)
//        return audio_out_sdl_->Output(pcm_buf, size);
//    else
//        return RET_FAIL;
    return RET_OK;
}

void AudioDecodeLoop::handle(int what, MsgBaseObj *data)
{
    if(what == RTMP_BODY_AUD_SPEC)
    {

    }
    else if(what == RTMP_BODY_AUD_RAW)
    {
        if(decode_frames_++ < PRINT_MAX_FRAME_DECODE_TIME) {
            AVPlayTime *play_time = AVPlayTime::GetInstance();
            LogInfo("%s:c:%u:t:%u", play_time->getAcodecTag(),
                    decode_frames_, play_time->getCurrenTime());
        }
        AudioRawMsg *aud_raw = (AudioRawMsg *)data;
        pcm_buf_size_ = PCM_BUF_MAX_SIZE;
        // 可以发送adts header, 如果不发送adts则要初始化 ctx的参数
        if(aac_decoder_->Decode(aud_raw->data , aud_raw->size ,
                                pcm_buf_, pcm_buf_size_) == RET_OK)
        {
            if(_callable_post_pcm_)
                _callable_post_pcm_(pcm_buf_, pcm_buf_size_, aud_raw->pts);
        } else {
            LogWarn("decode no got frame");
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
