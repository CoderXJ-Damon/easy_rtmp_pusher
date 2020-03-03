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

bool AACDecoder::SetConfig(const uint8_t *data, const size_t size)
{
    return true;
}
}
