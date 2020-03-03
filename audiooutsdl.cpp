#include "audiooutsdl.h"
#include "dlog.h"
#include "timeutil.h"
namespace LQF {

static void fill_audio_pcm(void *udata, Uint8 *stream, int len)
{
    static int64_t s_pre_time = TimesUtil::GetTimeMillisecond();
    AudioOutSDL *audio_out = (AudioOutSDL *)udata;

    int64_t cur_time = TimesUtil::GetTimeMillisecond();
    LogInfo("callback fill audio t:%ld", cur_time - s_pre_time);
    s_pre_time = cur_time;
    audio_out->lock_.lock();
    if(audio_out->ring_buffer_->getLength() < len) // 数据读取完毕
    {
        LogError("getLength() < len, buf_size:%d",  audio_out->ring_buffer_->getLength());
        audio_out->lock_.unlock();
        return;
    }

    SDL_memset(stream, 0, len);

    int ret = audio_out->ring_buffer_->read((char *)stream, len);     //
    LogError("size:%d, read:%d, buf_size:%d", len, ret, audio_out->ring_buffer_->getLength());
    audio_out->lock_.unlock();
}

AudioOutSDL::AudioOutSDL()
{

}

AudioOutSDL::~AudioOutSDL()
{
    LogInfo("~AudioOutSDL()");
    if(ring_buffer_)
    {
        Release();
    }
}
RET_CODE AudioOutSDL::Init(const Properties &properties)
{
    sample_rate_ = properties.GetProperty("sample_rate", 48000);
    sample_fmt_ = properties.GetProperty("sample_fmt", AUDIO_S16SYS);
    channels_ = properties.GetProperty("channels", 2);

    ring_buffer_ = new RingBuffer(1024 * 4 * 40); // 最多存储4帧数据
    //SDL initialize
    if(SDL_Init(SDL_INIT_AUDIO))    // 支持AUDIO
    {
        LogError("Could not initialize SDL - %s\n", SDL_GetError());
        return RET_FAIL;
    }
    // 音频参数设置SDL_AudioSpec
    wanted_spec.freq = sample_rate_;          // 采样频率
    wanted_spec.format = sample_fmt_; // 采样点格式
    wanted_spec.channels = channels_;          // 2通道
    wanted_spec.silence = 0;
    wanted_spec.samples = 1024;       // 每次读取的采样数量
    wanted_spec.callback = fill_audio_pcm; // 回调函数
    wanted_spec.userdata = this;
    SDL_AudioSpec  spec;
    //打开音频设备
    audio_dev = SDL_OpenAudioDevice(NULL, 0, &wanted_spec, &spec,
                                    SDL_AUDIO_ALLOW_FREQUENCY_CHANGE | SDL_AUDIO_ALLOW_CHANNELS_CHANGE);
    if(0 == audio_dev)
    {
        LogError("SDL_OpenAudioDevice failed - %s", SDL_GetError());
        return RET_FAIL;
    }
    if (spec.format != AUDIO_S16SYS)
    {
        LogError("SDL advised audio format %d is not supported!", spec.format);
        return RET_FAIL;
    }
    if (spec.channels != wanted_spec.channels)
    {
        LogError("SDL advised channel count %d is not supported!", spec.channels);
        return RET_FAIL;
    }
    //play audio
    SDL_PauseAudio(0);
    return RET_OK;
}
RET_CODE AudioOutSDL::Output(const uint8_t *pcm_buf, const uint32_t size)
{
    lock_.lock();
    int ret = ring_buffer_->write((char *)pcm_buf, size);
    if(ret != size)
    {
        // LogError("size:%d, write:%d", size, ret);
    }
    lock_.unlock();
    return RET_OK;
}

void AudioOutSDL::Release()
{
    // 关闭清理
    // 关闭音频设备
    if(audio_dev != 0)
    {
        SDL_PauseAudio(1);
        SDL_CloseAudioDevice(audio_dev);
        audio_dev = 0;
    }
    if(ring_buffer_)
    {    delete ring_buffer_;
        ring_buffer_ = NULL;
    }
}


}
