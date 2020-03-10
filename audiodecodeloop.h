/**
  包含audio 解码器,
**/
#ifndef AUDIODECODELOOP_H
#define AUDIODECODELOOP_H
#include <deque>
#include <mutex>
#include "aacdecoder.h"
#include "commonlooper.h"
#include "mediabase.h"
#include "packetqueue.h"

namespace LQF {
class AudioDecodeLoop : public CommonLooper
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
    void addFrameCallback(std::function<void(void*)> callableObject)
    {
        _callable_post_frame_ = callableObject;
    }
     virtual void Loop();
    void Post(void *);
private:

    AACDecoder *aac_decoder_ = NULL;
    std::function<void(uint8_t*, uint32_t, int64_t)> _callable_post_pcm_ = NULL;
    std::function<void(void *)> _callable_post_frame_ = NULL;
    uint8_t *pcm_buf_;
    int32_t pcm_buf_size_;
    const int PCM_BUF_MAX_SIZE = 32768; //

    uint32_t PRINT_MAX_FRAME_DECODE_TIME = 5;
    uint32_t decode_frames_ = 0;
    uint32_t deque_max_size_ = 20;
    PacketQueue *pkt_queue_;

    int		pkt_serial;         // 包序列
    int packet_cache_delay_ = 0;
};

}

#endif // AUDIODECODELOOP_H
