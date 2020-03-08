/**
  包含audio 解码器,
**/
#ifndef AUDIODECODELOOP_H
#define AUDIODECODELOOP_H
#include "aacdecoder.h"
#include "looper.h"
#include "mediabase.h"
namespace LQF {
class AudioDecodeLoop : public Looper
{
public:
    AudioDecodeLoop();
    virtual ~AudioDecodeLoop();
    RET_CODE Init(const Properties& properties);
    RET_CODE Output(const uint8_t *pcm_buf, const uint32_t size);
    void addCallback(std::function<void(uint8_t*, uint32_t, int64_t)> callableObject)
    {
        _callable_post_pcm_ = callableObject;
    }
    virtual void handle(int what, MsgBaseObj *data);
private:

    AACDecoder *aac_decoder_ = NULL;
    std::function<void(uint8_t*, uint32_t, int64_t)> _callable_post_pcm_ = NULL;
    uint8_t *pcm_buf_;
    int32_t pcm_buf_size_;
    const int PCM_BUF_MAX_SIZE = 32768; //

    uint32_t PRINT_MAX_FRAME_DECODE_TIME = 5;
    uint32_t decode_frames_ = 0;
};

}

#endif // AUDIODECODELOOP_H
