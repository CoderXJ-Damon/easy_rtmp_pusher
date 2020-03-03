#include "VideoDecodeLoop.h"
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
    if(video_out_sdl_)
        delete video_out_sdl_;
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
    if(video_out_sdl_)
        return video_out_sdl_->Output(video_buf, size);
    else
        return RET_FAIL;
}

void VideoDecodeLoop::handle(int what, MsgBaseObj *data)
{
    if(what == RTMP_BODY_METADATA)
    {
        if(!video_out_sdl_)
        {
            video_out_sdl_ = new VideoOutSDL();
            if(!video_out_sdl_)
            {
                LogError("new VideoOutSDL() failed");
                return;
            }
            Properties vid_out_properties;
            FLVMetadataMsg *metadata = (FLVMetadataMsg *)data;
            vid_out_properties.SetProperty("video_width", metadata->width);
            vid_out_properties.SetProperty("video_height",  metadata->height);
            vid_out_properties.SetProperty("win_x", 1000);
            vid_out_properties.SetProperty("win_title", "pull video display");
            delete metadata;
            if(video_out_sdl_->Init(vid_out_properties) != RET_OK)
            {
                LogError("video_out_sdl Init failed");
                return;
            }
        }

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
        NaluStruct *nalu = (NaluStruct *)data;
        yuv_buf_size_ = YUV_BUF_MAX_SIZE;
        if(h264_decoder_->Decode(nalu->data, nalu->size,
                                        yuv_buf_, yuv_buf_size_) == RET_OK)
        {
            callable_object_(yuv_buf_, yuv_buf_size_);
        }
        delete nalu;     // 要手动释放资源
    }
}
}
