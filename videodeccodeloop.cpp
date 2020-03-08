#include "VideoDecodeLoop.h"
#include "avtimebase.h"
namespace LQF
{
VideoDecodeLoop::VideoDecodeLoop()
{

}

VideoDecodeLoop::~VideoDecodeLoop()
{
    Stop();
    if(h264_decoder_)
        delete h264_decoder_;
    if(yuv_buf_)
        delete [] yuv_buf_;
}

RET_CODE VideoDecodeLoop::Init(const Properties &properties)
{
    h264_decoder_ = new H264Decoder();
    if(!h264_decoder_)
    {
        LogError("new H264Decoder() failed");
        return RET_ERR_OUTOFMEMORY;
    }
    Properties properties2;
    if(h264_decoder_->Init(properties2) != RET_OK)
    {
        LogError("aac_decoder_ Init failed");
        return RET_FAIL;
    }
    yuv_buf_ = new uint8_t[YUV_BUF_MAX_SIZE];
    if(!yuv_buf_)
    {
        LogError("yuv_buf_ new failed");
        return RET_ERR_OUTOFMEMORY;
    }
    return RET_OK;
}

RET_CODE VideoDecodeLoop::Output(const uint8_t *video_buf, const uint32_t size)
{
//    if(video_out_sdl_)
//        return video_out_sdl_->Output(video_buf, size);
//    else
//        return RET_FAIL;
    return RET_OK;
}

void VideoDecodeLoop::handle(int what, MsgBaseObj *data)
{
    if(what == RTMP_BODY_METADATA)
    {

    }
    else if(what == RTMP_BODY_VID_CONFIG)
    {
        VideoSequenceHeaderMsg *vid_config = (VideoSequenceHeaderMsg *)data;

        // 把sps送给解码器
        yuv_buf_size_ = YUV_BUF_MAX_SIZE;
        h264_decoder_->Decode(vid_config->sps_, vid_config->sps_size_,
                                        yuv_buf_, yuv_buf_size_);
        // 把pps送给解码器
        yuv_buf_size_ = YUV_BUF_MAX_SIZE;
        h264_decoder_->Decode(vid_config->pps_, vid_config->pps_size_,
                                        yuv_buf_, yuv_buf_size_);
        delete vid_config;
    }
    else
    {
        if(decode_frames_++ < PRINT_MAX_FRAME_DECODE_TIME) {
            AVPlayTime *play_time = AVPlayTime::GetInstance();
            LogInfo("%s:c:%u:t:%u", play_time->getAcodecTag(),
                    decode_frames_, play_time->getCurrenTime());
        }
        NaluStruct *nalu = (NaluStruct *)data;
        yuv_buf_size_ = YUV_BUF_MAX_SIZE;
        // 这里暂且只考虑没有B帧的情况
        if(h264_decoder_->Decode(nalu->data, nalu->size,
                                        yuv_buf_, yuv_buf_size_) == RET_OK)
        {
            callable_post_yuv_(yuv_buf_, yuv_buf_size_, nalu->pts);
        } else {
            LogWarn("decode no got frame");
        }
        delete nalu;     // 要手动释放资源
    }
}
}
