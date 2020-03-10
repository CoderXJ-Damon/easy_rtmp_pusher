#include <stdio.h>
#include <iostream>
#include <memory>
#include "dlog.h"
//#include "audio.h"
//#include "aacencoder.h"
//#include "audioresample.h"
using namespace std;

extern "C" {
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
#include <libavutil/opt.h>
#include <libavutil/audio_fifo.h>
}

//#include "pushwork.h"
#include "pullwork.h"
using namespace LQF;
extern int rtmpPull(int argc, char* argv[]);
extern int rtmpPublish(int argc, char* argv[]);
extern int testAacEncoder(const char *pcmFileName, const char* aacFileName);
#undef main

//#define RTMP_URL "rtmp://111.229.231.225/live/livestream"
#define RTMP_URL "rtmp://192.168.1.12/live/livestream"
// ffmpeg -re -i  rtmp_test_hd.flv  -vcodec copy -acodec copy  -f flv -y rtmp://111.229.231.225/live/livestream
//// ffmpeg -re -i  1920x832_25fps.flv  -vcodec copy -acodec copy  -f flv -y rtmp://111.229.231.225/live/livestream

std::mutex mtx; // 全局相互排斥锁.
std::condition_variable cv; // 全局条件变量.
bool ready = false; // 全局标志位.

void do_print_id(int id)
{
    std::unique_lock <std::mutex> lck(mtx);
    while (!ready) // 假设标志位不为 true, 则等待...
        cv.wait(lck); // 当前线程被堵塞, 当全局标志位变为 true 之后,
    // 线程被唤醒, 继续往下运行打印线程编号id.
    std::cout << "thread " << id << '\n';
}

void go()
{
    std::unique_lock <std::mutex> lck(mtx);

    cv.notify_one(); // 唤醒全部线程.
}
int main(int argc, char* argv[])
{
    init_logger("rtmp_push.log", S_INFO);
    {
        PullWork pullwork;
        Properties   pull_properties;
        pull_properties.SetProperty("rtmp_url", RTMP_URL);
        pull_properties.SetProperty("video_out_width", 720);
        pull_properties.SetProperty("video_out_height", 480);
        pull_properties.SetProperty("audio_out_sample_rate", 44100);

        pull_properties.SetProperty("cache_time", 1000);    //缓存时间

        if(pullwork.Init(pull_properties) != RET_OK)
        {
            LogError("pullwork.Init failed");
        }


        int count = 0;
        while (true) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            //            if(count++ > 1200)
            //                break;
        }

    } // 测试析构
    LogError("~~析构");



        std::thread threads[10];
         go(); // go!
          go(); // go!
        // spawn 10 threads:
        for (int i = 0; i < 10; ++i)
            threads[i] = std::thread(do_print_id, i);

        std::cout << "10 threads ready to race...\n";
        ready = true; // 设置全局标志位为 true.
//        go(); // go!
//        go(); // go!

      for (auto & th:threads)
            th.join();



    getchar();
    return 0;
}



