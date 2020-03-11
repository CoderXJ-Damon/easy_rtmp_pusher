#include "audiodecodeloop.h"
#include "avtimebase.h"
namespace LQF {
AudioDecodeLoop::AudioDecodeLoop()
{

}

AudioDecodeLoop::~AudioDecodeLoop()
{
    pkt_queue_->packet_queue_abort();       // 请求退出队列
    request_exit_ = true;

    Stop();
    if(aac_decoder_)
        delete aac_decoder_;
    if(pcm_buf_)
        delete [] pcm_buf_;

}

RET_CODE AudioDecodeLoop::Init(const Properties &properties)
{
    cache_duration_ = properties.GetProperty("cache_duration", 500);

    aac_decoder_ = new AACDecoder();
    if(!aac_decoder_)
    {
        LogError("new AACDecoder() failed");
        return RET_ERR_OUTOFMEMORY;
    }
    Properties properties2;
    if(aac_decoder_->Init(properties2) != RET_OK)
    {
        LogError("aac_decoder_ Init failed");
        return RET_FAIL;
    }
    pcm_buf_ = new uint8_t[PCM_BUF_MAX_SIZE];
    if(!pcm_buf_)
    {
        LogError("pcm_buf_ new failed");
        return RET_ERR_OUTOFMEMORY;
    }

    pkt_queue_ = new PacketQueue();
    if(!pkt_queue_)
    {
        LogError("PacketQueue new failed");
        return RET_ERR_OUTOFMEMORY;
    }
    if(pkt_queue_->packet_queue_init() == 0) {
        pkt_queue_->packet_queue_start();
        return RET_OK;
    }
    else {
        return RET_FAIL;
    }
}

RET_CODE AudioDecodeLoop::Output(const uint8_t *pcm_buf, const uint32_t size)
{
    //    if(audio_out_sdl_)
    //        return audio_out_sdl_->Output(pcm_buf, size);
    //    else
    //        return RET_FAIL;
    return RET_OK;
}

void AudioDecodeLoop::Loop()
{

    AVFrame *frame = av_frame_alloc();
    RET_CODE ret = RET_OK;
    while(true)
    {
        if(!cache_enough_) {
            // 等等触发
            semaphore_.wait();
            LogInfo("get post");
        }
        if(request_exit_)
            break;

        do
        {
            if(request_exit_)
                break;
            ret = aac_decoder_->ReceiveFrame(frame);
            if (ret >= 0) {
                frame->pts = frame->best_effort_timestamp;
            }

            if (ret == RET_OK)           // 解到一帧数据
            {
                if(decode_frames_++ < PRINT_MAX_FRAME_DECODE_TIME) {
                    AVPlayTime *play_time = AVPlayTime::GetInstance();
                    LogInfo("%s:c:%u:s:%d:t:%u", play_time->getAcodecTag(),
                            decode_frames_, pkt_queue_->get_nb_packets(),
                            play_time->getCurrenTime());
                }
                //                LogInfo("apts:%lld, dts:%lld", frame->pts, frame->pkt_dts);
                if(_callable_post_frame_)
                    _callable_post_frame_(frame);
            }
        } while (ret == RET_OK);
        if(request_exit_)
            break;
        AVPacket pkt;
        if (pkt_queue_->packet_queue_get(&pkt, 1, &pkt_serial) < 0)
        {
            LogError("packet_queue_get failed");
            break;
        }
        if (pkt.data != NULL && pkt.size!=0)
        {
            if(aac_decoder_->SendPacket(&pkt) != RET_OK)
                LogError("SendPacket failed, which is an API violation.\n");
            av_packet_unref(&pkt);
        } else {
            LogWarn("pkt null");
        }
    }
    av_frame_free(&frame);
    LogWarn("Loop leave");
}

void AudioDecodeLoop::Post(void *pkt)
{
    int64_t duration = pkt_queue_->get_duration();
    auto size = pkt_queue_->get_nb_packets();
    if(size > 15) {
        if(packet_cache_delay_++ > 5) {
            packet_cache_delay_ = 0; // 只是为了降低打印的频率
            LogInfo("cache %d packet lead to delay:%lld \n", size, duration);
        }
    }
    pkt_queue_->packet_queue_put((AVPacket *)pkt);

    if(!cache_enough_ ) {

        if(duration > cache_duration_)  {
            cache_enough_ = true;
            LogInfo("cache_duration = %d, post loop", duration);
            // 触发
            semaphore_.post();  // 唤醒解码线程
        }
    }
}

}
