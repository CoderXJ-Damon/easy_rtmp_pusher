#ifndef RTMPPLAYER_H
#define RTMPPLAYER_H
#include <functional>
#include <thread>
#include <vector>
#include "rtmpbase.h"
#include "mediabase.h"

namespace LQF {
class RTMPPlayer : public RTMPBase
{
public:
    RET_CODE Start();
    void Stop();
	void*  readPacketThread();
    // 收到数据包调用回调
    void AddAudioCallback(std::function<void(int what, MsgBaseObj *data, bool flush)> callable_object)
    {
        audio_callable_object_ = callable_object;
    }
    // 收到数据包调用回调
    void AddVideoCallback(std::function<void(int what, MsgBaseObj *data, bool flush)> callable_object)
    {
        video_callable_object_ = callable_object;
    }
public:
    RTMPPlayer();
    virtual ~RTMPPlayer();
private:
    
    void parseScriptTag(RTMPPacket &packet);
    bool request_exit_thread_ = false;
    std::thread *worker_ = NULL;
    std::function<void(int what, MsgBaseObj *data, bool flush)> audio_callable_object_ = NULL;
    std::function<void(int what, MsgBaseObj *data, bool flush)> video_callable_object_ = NULL;
private:
    //video and audio info
    int video_codec_id = 0;
    int video_width = 0;
    int video_height = 0;
    int video_frame_rate = 0;
    int audio_codec_id = 0;
    int audio_sample_rate = 0;
    int audio_bit_rate = 0;
    int audio_sample_size = 0;
    int audio_channel = 2;
    int file_size = 0;

    uint8_t profile_ = 0;
    uint8_t sample_frequency_index_ = 0;
    uint8_t channels_ = 0;
    std::vector<std::string> sps_vector_;    // 可以存储多个sps
    std::vector<std::string> pps_vector_;    // 可以存储多个pps

    int64_t audio_pre_pts_ = -1;
    int64_t video_pre_pts_ = -1;
};
}


#endif // RTMPPLAYER_H
