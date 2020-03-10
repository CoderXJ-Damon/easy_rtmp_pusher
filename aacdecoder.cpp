#include "aacdecoder.h"
#include "dlog.h"
namespace LQF {
AACDecoder::AACDecoder()
{

}

AACDecoder::~AACDecoder()
{
    //Check
    if (ctx)
    {
        //Close
        avcodec_close(ctx);
        av_free(ctx);
    }

    if (packet)
        //Release pacekt
        av_packet_free(&packet);
    if (frame)
        //Release frame
        av_frame_free(&frame);
}

RET_CODE AACDecoder::Init(const Properties &properties)
{
    //NO ctx yet
    ctx = NULL;


    // Get encoder
    codec = avcodec_find_decoder(AV_CODEC_ID_AAC);

    // Check codec
    if(!codec)
    {
        LogError("No codec found\n");
        return RET_ERR_MISMATCH_CODE;
    }
    //Alocamos el conto y el picture
    ctx = avcodec_alloc_context3(codec);

    //Set params
    //    ctx->request_sample_fmt		= AV_SAMPLE_FMT_S16;
    // 初始化codecCtx
    ctx->codec_type = AVMEDIA_TYPE_AUDIO;
    ctx->sample_rate = 16000;
    ctx->channels = 2;
    //    ctx->bit_rate = bit;
    ctx->channel_layout = AV_CH_LAYOUT_STEREO;
    //OPEN it
    if (avcodec_open2(ctx, codec, NULL) < 0)
    {
        LogError("could not open codec\n");
        return RET_FAIL;
    }

    //Create packet
    packet = av_packet_alloc();
    //Allocate frame
    frame = av_frame_alloc();

    //Get the number of samples
    numFrameSamples = 1024;
    return RET_OK;
}

RET_CODE AACDecoder::Decode(const uint8_t *in, int inLen, uint8_t *out, int &outLen)
{
    //If we have input
    if (inLen<=0)
        return RET_FAIL;

    //Set data
    packet->data = (uint8_t *)in;
    packet->size = inLen;

    //Decode it
    if (avcodec_send_packet(ctx, packet)<0)
        //nothing
    {
        LogError("-AACDecoder::Decode() Error decoding AAC packet");
        return RET_FAIL;
    }
    //Release side data
    av_packet_free_side_data(packet);

    //If we got a frame
    if (avcodec_receive_frame(ctx, frame)<0)
        //Nothing yet
    {
        outLen = 0;
        return RET_FAIL;
    }
    //Get data
    //1024
    float *buffer1 = (float *) frame->data[0];  //  LLLLL float 32bit [-1~1]
    float *buffer2 = (float *) frame->data[1];  // RRRRRR
    auto len = frame->nb_samples;
    int16_t *sample = (int16_t *)out;
    //Convert to SWORD
    for (size_t i=0; i<len; ++i)
    {
        // lrlrlr
        sample[i*2] =  (int16_t)(buffer1[i] * 0x7fff);
        sample[i*2 + 1] =  (int16_t)(buffer2[i] * 0x7fff);
    }
    outLen = 4096;
    static FILE *dump_pcm = NULL;
    if(!dump_pcm)
    {
        dump_pcm = fopen("aac_dump.pcm", "wb");
        if(!dump_pcm)
        {
            LogError("fopen aac_dump.pcm failed");
        }
    }
    if(dump_pcm)
    {//ffplay -ar 48000 -ac 2 -f s16le -i aac_dump.pcm
        fwrite(out, 1,outLen,  dump_pcm);
        fflush(dump_pcm);
    }
    //Return number of samples
    return RET_OK;
}

/**
 * @brief AACDecoder::SendPacket
 * @param avpkt
 * @return RET_OK:  on success, otherwise negative error code:
 *     RET_ERR_EAGAIN:   input is not accepted in the current state - user
 *                         must read output with avcodec_receive_frame() (once
 *                         all output is read, the packet should be resent, and
 *                         the call will not fail with EAGAIN).
 *      RET_ERR_EOF:       the decoder has been flushed, and no new packets can
 *                         be sent to it (also returned if more than 1 flush
 *                         packet is sent)
 *      AVERROR(EINVAL):   codec not opened, it is an encoder, or requires flush
 *      AVERROR(ENOMEM):   failed to add packet to internal queue, or similar
 *      other errors: legitimate decoding errors
 */
RET_CODE AACDecoder::SendPacket(const AVPacket *avpkt)
{
    int ret = avcodec_send_packet(ctx, avpkt);
    if(0 == ret)
        return RET_OK;

    if (AVERROR(EAGAIN) == ret) {
//        LogWarn("avcodec_send_packet failed, RET_ERR_EAGAIN.\n");
        return RET_ERR_EAGAIN;
    } else if(AVERROR_EOF) {
        LogWarn("avcodec_send_packet failed, RET_ERR_EOF.\n");
        return RET_ERR_EOF;
    } else {
        LogError("avcodec_send_packet failed, AVERROR(EINVAL) or AVERROR(ENOMEM) or other...\n");
        return RET_FAIL;
    }
}
/**
 * @brief AACDecoder::ReceiveFrame
 * @param frame
 *  *      RET_OK:                 success, a frame was returned
 *      RET_ERR_EAGAIN:   output is not available in this state - user must try
 *                         to send new input
 *      AVERROR_EOF:       the decoder has been fully flushed, and there will be
 *                         no more output frames
 *      AVERROR(EINVAL):   codec not opened, or it is an encoder
 *      other negative values: legitimate decoding errors
 */
RET_CODE AACDecoder::ReceiveFrame(AVFrame *frame)
{
    int ret = avcodec_receive_frame(ctx, frame);
    if(0 == ret)
        return RET_OK;

    if (AVERROR(EAGAIN) == ret) {
//        LogWarn("avcodec_receive_frame failed, RET_ERR_EAGAIN.\n");
        return RET_ERR_EAGAIN;
    } else if(AVERROR_EOF) {
        LogWarn("avcodec_receive_frame failed, RET_ERR_EOF.\n");
        return RET_ERR_EOF;
    } else {
        LogError("avcodec_receive_frame failed, AVERROR(EINVAL) or AVERROR(ENOMEM) or other...\n");
        return RET_FAIL;
    }
}

bool AACDecoder::SetConfig(const uint8_t *data, const size_t size)
{
    return true;
}
}
