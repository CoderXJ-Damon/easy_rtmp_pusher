/**
  包含audio 解码器,
**/
#ifndef AUDIODECODELOOP_H
#define AUDIODECODELOOP_H
#include "aacdecoder.h"
#include "looper.h"
#include "mediabase.h"
#include "audiooutsdl.h"
namespace LQF {
class AudioDecodeLoop : public Looper
{
public:
    AudioDecodeLoop();
    virtual ~AudioDecodeLoop();
    RET_CODE Init(const Properties& properties);
    RET_CODE Output(const uint8_t *pcm_buf, const uint32_t size);
    void addCallback(std::function<void(uint8_t*, int32_t)> callableObject)
    {
        _callableObject = callableObject;
    }
private:
    virtual void handle(int what, MsgBaseObj *data);
    AACDecoder *aac_decoder_ = NULL;
    std::function<void(uint8_t*, int32_t)> _callableObject = NULL;
    uint8_t *pcm_buf_;
    int32_t pcm_buf_size_;
    const int PCM_BUF_MAX_SIZE = 32768; //
    AudioOutSDL *audio_out_sdl_ = NULL;
};

}

#endif // AUDIODECODELOOP_H
