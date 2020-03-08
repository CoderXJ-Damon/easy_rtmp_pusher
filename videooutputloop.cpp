#include "videooutputloop.h"
#include "dlog.h"
#include "avsync.h"
#include "avtimebase.h"
namespace LQF {
VideoOutputLoop::VideoOutputLoop()
    :CommonLooper()
{

}

RET_CODE VideoOutputLoop::Init(const Properties &properties)
{
    int size = properties.GetProperty("size", VIDEO_PICTURE_QUEUE_SIZE);
    frame_queue_ = new FrameQueue();
    if(!frame_queue_) {
        LogError("new FrameQueue(%d) failed", size);
        return RET_ERR_OUTOFMEMORY;
    }
    RET_CODE ret = frame_queue_->Init(size, 1);
    if(ret != RET_OK) {
        LogError("Init failed, ret:%d", ret);
    }
    return ret;
}

void VideoOutputLoop::Loop()
{
    while(true)
    {
        if(request_exit_)
            break;
        // 读取帧
        Frame *frame = frame_queue_->PeekReadable();
        if(frame) {
            AV_FRAME_SYNC_RESULT sync_result = (AV_FRAME_SYNC_RESULT)callback_avsync_(
                        frame->pts_, frame->duration_);
            if(AV_FRAME_PLAY == sync_result || AV_FRAME_FREE_RUN || !is_show_first_frame_) {
                // 打印第一帧的时间
                if(!is_show_first_frame_) {
                    AVPlayTime *play_time = AVPlayTime::GetInstance();
                    LogInfo("%s:t:%u", play_time->getVoutTag(), play_time->getCurrenTime());
                    is_show_first_frame_ = true;
                }
                // 格式默认是YUV420p先
                callback_display_(frame->Data(), frame->Size(), frame->duration_);
            }
            // 不是等待则释放帧
            if(sync_result != AV_FRAME_HOLD) {
                frame_queue_->Next();   // 释放一帧
            }
        } else {
            LogWarn("PeekReadable is null");
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    LogWarn("Loop exit");
}

RET_CODE VideoOutputLoop::PushFrame(uint8_t *data, uint32_t size, int64_t pts)
{
    if(frame_queue_) {
        Frame *frame = frame_queue_->PeekWritable();
        if(!frame) {
            LogError("PeekWritable failed");
            return RET_FAIL;
        }
        if(frame->Init(data, size, pts) != RET_OK) {
            LogError("Frame Init failed");
            return RET_FAIL;
        }
        int duration = 40;
//        if(pre_pts_ != 0) {

//            if(pts > pre_pts_) {
//                duration = pts > pre_pts_;
//                if(duration > 2000) {
//                    LogWarn("duration:%d may be error, force to %d", duration, 40);
//                    duration = 40;
//                }
//            }
//        }
        frame->duration_ = duration;    // 帧间隔
        pre_pts_ = pts;
        frame_queue_->Push();       // 真正插入一帧
        return RET_OK;
    } else {
        LogError("frame_queue_ is null");
        return RET_FAIL;
    }
}


}
