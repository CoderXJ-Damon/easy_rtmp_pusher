#ifndef VideoDecodeLoop_H
#define VideoDecodeLoop_H
#include "looper.h"
#include "mediabase.h"
#include "h264decoder.h"
namespace LQF {
class VideoDecodeLoop: public Looper
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
    virtual RET_CODE Output(const uint8_t *video_buf, const uint32_t size);
    virtual void handle(int what, MsgBaseObj *data);
private:

    H264Decoder *h264_decoder_ = NULL;
    std::function<void(uint8_t*, uint32_t, int64_t)> callable_post_yuv_ = NULL;
    uint8_t *yuv_buf_;
    int32_t yuv_buf_size_;
    const int YUV_BUF_MAX_SIZE = int(1920*1080*1.5); //先写死最大支持, fixme

    // 延迟指标监测
    uint32_t PRINT_MAX_FRAME_DECODE_TIME = 5;
    uint32_t decode_frames_ = 0;
};
}



#endif // VideoDecodeLoop_H
