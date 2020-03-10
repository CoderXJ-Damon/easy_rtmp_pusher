#ifndef VIDEODECODELOOP_H
#define VIDEODECODELOOP_H
#include "commonlooper.h"
#include "mediabase.h"
#include "h264decoder.h"
#include "packetqueue.h"
namespace LQF {
class VideoDecodeLoop: public CommonLooper
{
public:
    VideoDecodeLoop();
    virtual ~VideoDecodeLoop();
    virtual RET_CODE Init(const Properties& properties);
    int GetWdith(){
        return h264_decoder_->GetWidth();
    }
    int GetHeight(){
        return h264_decoder_->GetHeight();
    }
    void AddCallback(std::function<void(uint8_t*, uint32_t, int64_t)> callable_object)
    {
        callable_post_yuv_ = callable_object;
    }
    void AddFrameCallback(std::function<void(void*)> callable_object)
    {
        callable_post_frame_ = callable_object;
    }
    virtual RET_CODE Output(const uint8_t *video_buf, const uint32_t size);
    virtual void Loop();
    void Post(void *);
private:


    H264Decoder *h264_decoder_ = NULL;
    std::function<void(uint8_t*, uint32_t, int64_t)> callable_post_yuv_ = NULL;
    std::function<void(void *)> callable_post_frame_ = NULL;
    uint8_t *yuv_buf_;
    int32_t yuv_buf_size_;
    const int YUV_BUF_MAX_SIZE = int(1920*1080*1.5); //先写死最大支持, fixme

    // 延迟指标监测
    uint32_t PRINT_MAX_FRAME_DECODE_TIME = 5;
    uint32_t decode_frames_ = 0;
    uint32_t deque_max_size_ = 20;
    PacketQueue *pkt_queue_;

    int		pkt_serial;         // 包序列
    int packet_cache_delay_;
};
}



#endif // VideoDecodeLoop_H
