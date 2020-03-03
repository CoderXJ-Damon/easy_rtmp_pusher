#ifndef AUDIOOUTSDL_H
#define AUDIOOUTSDL_H

#include <mutex>
#include <SDL.h>
#include "mediabase.h"
#include "ringbuffer.h"
namespace LQF {
class AudioOutSDL
{
public:
    AudioOutSDL();
    virtual ~AudioOutSDL();
    /**
     * @brief Init
     * @param   "sample_fmt", 采样格式 AVSampleFormat对应的值，缺省为AV_SAMPLE_FMT_S16
     *          "sample_rate", 采样率，缺省为480000
     *          "channels",  采样通道，缺省为2通道
     * @return
     */
    virtual RET_CODE Init(const Properties &properties);
    virtual RET_CODE Output(const uint8_t *pcm_buf, const uint32_t size);
    virtual void Release();
     RingBuffer *ring_buffer_;
     std::mutex lock_;//
private:
    SDL_AudioSpec wanted_spec;
    int sample_rate_ = 48000;
    int sample_fmt_ = AUDIO_S16SYS;
    int channels_ = 2;
    SDL_AudioDeviceID audio_dev = 0;
};
}
#endif // AUDIOOUTSDL_H
