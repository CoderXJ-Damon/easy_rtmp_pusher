#ifndef AVSYNC_H
#define AVSYNC_H

#include "libavutil/time.h"

#include "mediabase.h"


namespace LQF {
//const int64_t AV_NOSYNC_THRESHOLD = 40;
//enum {
//    AV_SYNC_AUDIO_MASTER, /* default choice */
//    AV_SYNC_VIDEO_MASTER,
//    AV_SYNC_EXTERNAL_CLOCK, /* synchronize to an external clock */
//};

//enum AV_SYNC {
//    AV_SYNC_FRAME_FREE_RUN, // 有就播放，不做同步
//    AV_SYNC_FRAME_HOLD,     // 等待到时间再播放
//    AV_SYNC_FRAME_DROP,     // 丢掉当前帧
//    AV_SYNC_FRAME_PLAY      // 正常播放
//};

//class AVSync;
//class Clock
//{
//public:
//    Clock(int *queue_serial)
//    {
//        speed_ = 1.0;
//        paused_ = 0;
//        queue_serial_ = queue_serial;
//        set_clock(NAN, -1);
//    }
//    double get_clock()
//    {
//        if (*queue_serial_ != serial_)
//            return NAN;
//        if (paused_) {
//            return pts_;
//        } else {
//            double time = av_gettime_relative() / 1000000.0;
//            return pts_drift_ + time - (time - last_updated_) * (1.0 - speed_);
//        }
//    }
//    void set_clock_at(double pts, int serial, double time)
//    {
//        pts_ = pts;
//        last_updated_ = time;
//        pts_drift_ = pts_ - time;
//        serial_ = serial;
//    }
//    void set_clock(double pts, int serial)
//    {
//        double time = av_gettime_relative() / 1000000.0;
//        set_clock_at(pts, serial, time);
//    }
//    void init_clock(Clock *c, int *queue_serial)
//    {
//        speed_ = 1.0;
//        paused_ = 0;
//        queue_serial_ = queue_serial;
//        set_clock(NAN, -1);
//    }
//    void sync_clock_to_slave(Clock *slave)
//    {
//        double clock = get_clock();
//        double slave_clock = slave->get_clock();
//        if (!isnan(slave_clock) && (isnan(clock) || fabs(clock - slave_clock) > AV_NOSYNC_THRESHOLD))
//            slave->set_clock(slave_clock, slave->serial_);
//    }
//    int get_master_sync_type(AVSync *avsync)
//    {
//        if (avsync->av_sync_type == AV_SYNC_VIDEO_MASTER) {
//            if (avsync->has_video)
//                return AV_SYNC_VIDEO_MASTER;
//            else
//                return AV_SYNC_AUDIO_MASTER;
//        } else if (avsync->av_sync_type == AV_SYNC_AUDIO_MASTER) {
//            if (avsync->has_audio)
//                return AV_SYNC_AUDIO_MASTER;
//            else
//                return AV_SYNC_EXTERNAL_CLOCK;
//        } else {
//            return AV_SYNC_EXTERNAL_CLOCK;
//        }
//    }
//    /* get the current master clock value */
//    static double get_master_clock(AVSync *avsync)
//    {
//        double val;

//        switch (get_master_sync_type(avsync)) {
//        case AV_SYNC_VIDEO_MASTER:
//            val = avsync->vidclk->get_clock();
//            break;
//        case AV_SYNC_AUDIO_MASTER:
//            val = avsync->audclk->get_clock();
//            break;
//        default:
//            val = avsync->extclk->get_clock();
//            break;
//        }
//        return val;
//    }
//public:
//    double pts_;           /* clock base */
//    double pts_drift_;     /* clock base minus time at which we updated the clock */
//    double last_updated_;
//    double speed_;
//    int serial_;           /* clock is based on a packet with this serial */
//    int paused_;
//    int *queue_serial_;    /* pointer to the current packet queue serial, used for obsolete clock*/
//};

//class AVSync
//{
//public:
//    AVSync()
//    {

//    }
//    ~AVSync()
//    {

//    }
//    void update_video_pts(double pts, int serial) {
//        vidclk->set_clock(pts, serial);
//    }
//    void update_audio_pts(double pts, int serial) {
//        audclk->set_clock(pts, serial);
//    }
//    AV_SYNC GetVideoSync()
//    {

//    }

//    Clock *audclk;
//    Clock *vidclk;
//    Clock *extclk;
//    int av_sync_type;
//    int has_audio = 0;
//    int has_video = 0;
//};

}

#endif // AVSYNC_H
