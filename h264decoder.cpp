#include "h264decoder.h"
/* 3 zero uint8_ts syncword */
static const uint8_t sync_uint8_ts[] = { 0, 0, 0, 1 };
H264Decoder::H264Decoder()
{
    codec_ = NULL;
    ctx_ = NULL;
}

H264Decoder::~H264Decoder()
{
    if (ctx_)
    {
        avcodec_free_context(&ctx_);
    }
    if (picture_)
        av_frame_free(&picture_);

}

RET_CODE H264Decoder::Init(const Properties &properties)
{


    picture_ = av_frame_alloc();
    if(!picture_)
    {
        LogError("av_frame_alloc failed\n");
        return RET_ERR_OUTOFMEMORY;
    }

    codec_ = avcodec_find_decoder(AV_CODEC_ID_H264);
    if(!codec_)
    {
        LogError("No decoder found\n");
        return RET_ERR_MISMATCH_CODE;
    }

    ctx_ = avcodec_alloc_context3(codec_);
    if(!ctx_)
    {
        LogError("avcodec_alloc_context3 failed\n");
        return RET_ERR_OUTOFMEMORY;
    }

    if(avcodec_open2(ctx_, codec_, NULL) != 0)
    {
        LogError("avcodec_open2 %s failed\n", codec_->name);
        avcodec_close(ctx_);
        free(ctx_);
        ctx_ = NULL;
        return RET_FAIL;
    }
}

RET_CODE H264Decoder::Decode(uint8_t *in, int32_t in_len, uint8_t *out, int32_t &out_len)
{
    int got_picture=0;

    AVPacket pkt;
    av_init_packet(&pkt);
    pkt.data = in;
    pkt.size = in_len;

    static FILE *dump_h264 = NULL;
    if(!dump_h264)
    {
        dump_h264 = fopen("dump_h2642.h264", "wb");
        if(!dump_h264)
        {
            LogError("fopen dump_h2642.h264 failed");
        }
    }
    if(dump_h264)
    {//ffplay -ar 48000 -ac 2 -f s16le -i aac_dump.pcm
        fwrite(in, 1,in_len,  dump_h264);//Y
        fflush(dump_h264);
    }
    int readed = avcodec_decode_video2(ctx_, picture_, &got_picture, &pkt);

    //Si hay picture_
    if (got_picture && readed>0)
    {
        if(ctx_->width==0 || ctx_->height==0)
        {
            LogError("-Wrong dimmensions [%d,%d]\n",ctx_->width,ctx_->height);
            return RET_FAIL;
        }
        int width = picture_->width;
        int height = picture_->height;
        out_len = picture_->width * picture_->height * 1.5;
        memcpy(out, picture_->data[0], picture_->width * picture_->height);
        memcpy(out + picture_->width * picture_->height, picture_->data[1],
                (picture_->width * picture_->height) /4);
        memcpy(out + (picture_->width * ctx_->height) + (picture_->width * ctx_->height) /4,
               picture_->data[2],
                (picture_->width * picture_->height) /4);
        for(int j=0; j<height; j++)
        {
            memcpy(out + j*width, picture_->data[0] + j * picture_->linesize[0], width);
        }
        out += width * height;
        for(int j=0; j<height/2; j++)
        {
            memcpy(out + j*width/2, picture_->data[1] + j * picture_->linesize[1], width/2);
        }
        out += width * height/2/2;
        for(int j=0; j<height/2; j++)
        {
            memcpy(out + j*width/2, picture_->data[2] + j * picture_->linesize[2], width/2);
        }


        static FILE *dump_yuv = NULL;
        if(!dump_yuv)
        {
            dump_yuv = fopen("h264_dump_320x240.yuv", "wb");
            if(!dump_yuv)
            {
                LogError("fopen h264_dump.yuv failed");
            }
        }
        if(dump_yuv)
        {//ffplay -ar 48000 -ac 2 -f s16le -i aac_dump.pcm
            //AVFrame存储YUV420P对齐分析
            //https://blog.csdn.net/dancing_night/article/details/80830920?depth_1-utm_source=distribute.pc_relevant.none-task&utm_source=distribute.pc_relevant.none-task
            for(int j=0; j<height; j++)
                fwrite(picture_->data[0] + j * picture_->linesize[0], 1, width, dump_yuv);
            for(int j=0; j<height/2; j++)
                fwrite(picture_->data[1] + j * picture_->linesize[1], 1, width/2, dump_yuv);
            for(int j=0; j<height/2; j++)
                fwrite(picture_->data[2] + j * picture_->linesize[2], 1, width/2, dump_yuv);

            fflush(dump_yuv);
        }
        return RET_OK;
    }
    out_len = 0;
    return RET_ERR_EAGAIN;
}

