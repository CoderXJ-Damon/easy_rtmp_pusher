#ifndef VideoDecodeLoop_H
#define VideoDecodeLoop_H
#include "looper.h"
#include "mediabase.h"
#include "h264decoder.h"
#include "videooutsdl.h"
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
    void AddCallback(std::function<void(uint8_t*, int32_t)> callable_object)
    {
        callable_object_ = callable_object;
    }
    virtual RET_CODE Output(const uint8_t *video_buf, const uint32_t size);
private:
    virtual void handle(int what, MsgBaseObj *data);
    H264Decoder *h264_decoder_ = NULL;
    std::function<void(uint8_t*, int32_t)> callable_object_ = NULL;
    uint8_t *yuv_buf_;
    int32_t yuv_buf_size_;
    const int YUV_BUF_MAX_SIZE = 1920*1080*1.5; //先写死最大支持, fixme
    VideoOutSDL *video_out_sdl_ = NULL;
};
}



#endif // VideoDecodeLoop_H
