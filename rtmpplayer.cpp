#include "rtmpplayer.h"

#include "librtmp/rtmp_sys.h"
#include "dlog.h"
#include "mediabase.h"
#include "timeutil.h"
namespace LQF
{
RTMPPlayer::RTMPPlayer():RTMPBase(RTMP_BASE_TYPE_PLAY)
{
    //    _decodeLoop = new DecodeLoop(window);
    //    _audioDecode = new AudioDecode();
}

RTMPPlayer::~RTMPPlayer()
{
    if(worker_)
    {
        Stop();
    }
}

void RTMPPlayer::parseScriptTag(RTMPPacket &packet)
{
    LogInfo("begin parse info %d",packet.m_nBodySize);
    AMFObject obj;
    AVal val;
    AMFObjectProperty * property;
    AMFObject subObject;
    if (AMF_Decode(&obj, packet.m_body, packet.m_nBodySize, FALSE) < 0)
    {
        LogInfo("%s, error decoding invoke packet", __FUNCTION__);
    }
    AMF_Dump(&obj);
    LogInfo(" amf obj %d",obj.o_num);
    for (int n = 0; n < obj.o_num; n++)
    {
        property = AMF_GetProp(&obj, NULL, n);
        if (property != NULL)
        {
            if (property->p_type == AMF_OBJECT)
            {
                AMFProp_GetObject(property, &subObject);
                for (int m = 0; m < subObject.o_num; m++)
                {
                    property = AMF_GetProp(&subObject, NULL, m);
                    LogInfo("val = %s",property->p_name.av_val);
                    if (property != NULL)
                    {
                        if (property->p_type == AMF_OBJECT)
                        {

                        }
                        else if (property->p_type == AMF_BOOLEAN)
                        {
                            int bVal = AMFProp_GetBoolean(property);
                            if (strncasecmp("stereo", property->p_name.av_val, property->p_name.av_len) == 0)
                            {
                                audio_channel =  bVal > 0 ? 2 : 1;
                                LogInfo("parse channel %d",audio_channel);
                            }
                        }
                        else if (property->p_type == AMF_NUMBER)
                        {
                            double dVal = AMFProp_GetNumber(property);
                            if (strncasecmp("width", property->p_name.av_val, property->p_name.av_len) == 0)
                            {
                                video_width = (int)dVal;
                                LogInfo("parse widht %d",video_width);
                            }
                            else if (strcasecmp("height", property->p_name.av_val) == 0)
                            {
                                video_height = (int)dVal;
                                LogInfo("parse Height %d",video_height);
                            }
                            else if (strcasecmp("framerate", property->p_name.av_val) == 0)
                            {
                                video_frame_rate = (int)dVal;
                                LogInfo("parse frame_rate %d",video_frame_rate);
                            }
                            else if (strcasecmp("videocodecid", property->p_name.av_val) == 0)
                            {
                                video_codec_id = (int)dVal;
                                LogInfo("parse video_codec_id %d",video_codec_id);
                            }
                            else if (strcasecmp("audiosamplerate", property->p_name.av_val) == 0)
                            {
                                audio_sample_rate = (int)dVal;
                                LogInfo("parse audiosamplerate %d",audio_sample_rate);
                            }
                            else if (strcasecmp("audiodatarate", property->p_name.av_val) == 0)
                            {
                                audio_bit_rate = (int)dVal;
                                LogInfo("parse audiodatarate %d",audio_bit_rate);
                            }
                            else if (strcasecmp("audiosamplesize", property->p_name.av_val) == 0)
                            {
                                audio_sample_size = (int)dVal;
                                LogInfo("parse audiosamplesize %d",audio_sample_size);
                            }
                            else if (strcasecmp("audiocodecid", property->p_name.av_val) == 0)
                            {
                                audio_codec_id = (int)dVal;
                                LogInfo("parse audiocodecid %d",audio_codec_id);
                            }
                            else if (strcasecmp("filesize", property->p_name.av_val) == 0)
                            {
                                file_size = (int)dVal;
                                LogInfo("parse filesize %d",file_size);
                            }
                        }
                        else if (property->p_type == AMF_STRING)
                        {
                            AMFProp_GetString(property, &val);
                        }
                    }
                }
            }
            else
            {
                AMFProp_GetString(property, &val);

                LogInfo("val = %s",val.av_val);
            }
        }
    }
}

RET_CODE RTMPPlayer::Start()
{
    if(!worker_)
    {
        worker_ = new std::thread(std::bind(&RTMPPlayer::readPacketThread,this));
        if(worker_ == NULL)
        {
            LogError("new std::thread failed");
            return RET_FAIL;
        }
    }
    return RET_OK;
}

void RTMPPlayer::Stop()
{
    request_exit_thread_ = true;
    if(worker_)
    {
        if(worker_->joinable())
        {
            worker_->join();
        }
        delete worker_;
        worker_ = NULL;
    }
}

void* RTMPPlayer::readPacketThread()
{
    //此处可以优化
    RTMPPacket packet = {0};
    int64_t cur_time = TimesUtil::GetTimeMillisecond();
    int64_t pre_time = cur_time;
    while (!request_exit_thread_)
    {
        //短线重连
        if(!IsConnect())
        {
            LogInfo("短线重连 re connect");
            if(!Connect(url_))      //重连失败
            {
                LogInfo("短线重连 reConnect fail %s",url_.c_str());
                msleep(10);
                continue;
            }
        }
        cur_time = TimesUtil::GetTimeMillisecond();
        int64_t t = cur_time - pre_time;
        pre_time = cur_time;
        RTMP_ReadPacket(rtmp_, &packet);
        int64_t diff = TimesUtil::GetTimeMillisecond() - cur_time;
        //        if(t>10 || diff>10)
        //            LogInfo("cur-pre t:%ld, ReadPacket:%ld", t, diff);
        if (RTMPPacket_IsReady(&packet))    // 检测是不是整个包组好了
        {
            diff = TimesUtil::GetTimeMillisecond() - cur_time;
            if(diff>10)
            {
                bool keyframe =false;
                if (packet.m_packetType == RTMP_PACKET_TYPE_VIDEO)
                {
                    keyframe = 0x17 == packet.m_body[0] ? true : false;
                }
                //                LogInfo("RTMPPacket_IsReady:%ld, keyframe:%d, type:%u,size:%u",
                //                        diff, keyframe, packet.m_packetType,
                //                        packet.m_nBodySize);
            }
            uint8_t nalu_header[4] = { 0x00, 0x00, 0x00, 0x01 };
            // Process packet, eg: set chunk size, set bw, ...
            //            RTMP_ClientPacket(m_pRtmp, &packet);
            if (!packet.m_nBodySize)
                continue;
            if (packet.m_packetType == RTMP_PACKET_TYPE_VIDEO)
            {
                // 解析完数据再发送给解码器
                // 判断起始字节, 检测是不是spec config, 还原出sps pps等
                // 重组帧
                bool keyframe = 0x17 == packet.m_body[0] ? true : false;
                bool sequence = 0x00 == packet.m_body[1];
                //                if(keyframe)
                //                {
                //                    LogInfo("keyframe=%s, sequence=%s, size:%d", keyframe ? "true" : "false",
                //                            sequence ? "true" : "false",
                //                            packet.m_nBodySize);
                //                }
                // SPS/PPS sequence
                if (sequence)
                {
                    uint32_t offset = 10;
                    uint32_t sps_num = packet.m_body[offset++] & 0x1f;
                    if(sps_num>0)
                    {
                        sps_vector_.clear();    // 先清空原来的缓存
                    }
                    for (int i = 0; i < sps_num; i++)
                    {
                        uint8_t ch0 = packet.m_body[offset];
                        uint8_t ch1 = packet.m_body[offset + 1];
                        uint32_t sps_len = ((ch0 << 8) | ch1);
                        offset += 2;
                        // Write sps data
                        std::string sps;
                        sps.append(nalu_header, nalu_header + 4); // 存储 start code
                        sps.append(packet.m_body + offset, packet.m_body + offset + sps_len);
                        sps_vector_.push_back(sps);
                        offset += sps_len;
                    }
                    uint32_t pps_num = packet.m_body[offset++] & 0x1f;
                    if(pps_num > 0)
                    {
                        pps_vector_.clear();    // 先清空原来的缓存
                    }
                    for (int i = 0; i < pps_num; i++)
                    {
                        uint8_t ch0 = packet.m_body[offset];
                        uint8_t ch1 = packet.m_body[offset + 1];
                        uint32_t pps_len = ((ch0 << 8) | ch1);
                        offset += 2;
                        // Write pps data
                        std::string pps;
                        pps.append(nalu_header, nalu_header + 4); // 存储 start code
                        pps.append(packet.m_body + offset, packet.m_body + offset + pps_len);
                        pps_vector_.push_back(pps);
                        offset += pps_len;
                    }
                    VideoSequenceHeaderMsg * vid_config_msg = new VideoSequenceHeaderMsg(
                                (uint8_t *)sps_vector_[0].c_str(),
                            sps_vector_[0].size(),
                            (uint8_t *)pps_vector_[0].c_str(),
                            pps_vector_[0].size()
                            );
                    video_callable_object_(RTMP_BODY_VID_CONFIG, vid_config_msg, false);
                }
                // Nalu frames
                else
                {

                    uint32_t offset = 5;
                    uint8_t ch0 = packet.m_body[offset];
                    uint8_t ch1 = packet.m_body[offset + 1];
                    uint8_t ch2 = packet.m_body[offset + 2];
                    uint8_t ch3 = packet.m_body[offset + 3];
                    uint32_t data_len = ((ch0 << 24) | (ch1 << 16) | (ch2 << 8) | ch3);
                    offset += 4;
                    NaluStruct * nalu = new NaluStruct(data_len + 4);
                    memcpy(nalu->data, nalu_header, 4);
                    memcpy(nalu->data + 4, packet.m_body + offset, data_len);

                    if(video_pre_pts_ == -1){
                        video_pre_pts_= packet.m_nTimeStamp;
                        if(!packet.m_hasAbsTimestamp) {
                            LogWarn("no init video pts");
                        }
                    }
                    else {
                        if(packet.m_hasAbsTimestamp)
                            video_pre_pts_= packet.m_nTimeStamp;
                        else
                            video_pre_pts_ += packet.m_nTimeStamp;
                    }
                    nalu->pts = video_pre_pts_;
                    video_callable_object_(RTMP_BODY_VID_RAW, nalu, false);
                    offset += data_len;
                }
            }
            else if (packet.m_packetType == RTMP_PACKET_TYPE_AUDIO)
            {
                static int64_t s_is_pre_ready = TimesUtil::GetTimeMillisecond();
                cur_time = TimesUtil::GetTimeMillisecond();
                //                 LogInfo("aud ready  t:%ld", cur_time - s_is_pre_ready);
                s_is_pre_ready = cur_time;

                bool sequence = (0x00 == packet.m_body[1]);
                //                LogInfo("sequence=%s\n", sequence ? "true" : "false");
                uint8_t format = 0, samplerate = 0, sampledepth = 0, type = 0;
                uint8_t frame_length_flag = 0, depend_on_core_coder = 0, extension_flag = 0;
                // AAC sequence
                if (sequence)
                {
                    format = (packet.m_body[0] & 0xf0) >> 4;
                    samplerate = (packet.m_body[0] & 0x0c) >> 2;
                    sampledepth = (packet.m_body[0] & 0x02) >> 1;
                    type = packet.m_body[0] & 0x01;
                    // sequence = packet.m_body[1];
                    // AAC(AudioSpecificConfig)
                    if (format == 10) {     // AAC格式
                        uint8_t ch0 = packet.m_body[2];
                        uint8_t ch1 = packet.m_body[3];
                        uint16_t config = ((ch0 << 8) | ch1);
                        profile_ = (config & 0xF800) >> 11;
                        sample_frequency_index_ = (config & 0x0780) >> 7;
                        channels_ = (config & 0x78) >> 3;
                        frame_length_flag = (config & 0x04) >> 2;
                        depend_on_core_coder = (config & 0x02) >> 1;
                        extension_flag = config & 0x01;
                    }
                    // Speex(Fix data here, so no need to parse...)
                    else if (format == 11) {    // MP3格式
                        // 16 KHz, mono, 16bit/sample
                        type = 0;
                        sampledepth = 1;
                        samplerate = 4;
                    }
                    audio_sample_rate = RTMPBase::GetSampleRateByFreqIdx(sample_frequency_index_);
                    AudioSpecMsg *aud_spec_msg = new  AudioSpecMsg(profile_,
                                                                   channels_,
                                                                   audio_sample_rate);
                    audio_callable_object_(RTMP_BODY_AUD_SPEC, aud_spec_msg, false);
                }
                // Audio frames
                else
                {
                    // ADTS(7 bytes) + AAC data
                    uint32_t data_len = packet.m_nBodySize - 2 + 7;
                    uint8_t adts[7];
                    adts[0] = 0xff;
                    adts[1] = 0xf9;
                    adts[2] = ((profile_ - 1) << 6) | (sample_frequency_index_ << 2) | (channels_ >> 2);
                    adts[3] = ((channels_ & 3) << 6) + (data_len >> 11);
                    adts[4] = (data_len & 0x7FF) >> 3;
                    adts[5] = ((data_len & 7) << 5) + 0x1F;
                    adts[6] = 0xfc;
                    // Write audio frames
                    AudioRawMsg *aud_raw = new  AudioRawMsg(data_len);
                    memcpy(aud_raw->data, adts, 7);
                    memcpy(aud_raw->data + 7, packet.m_body + 2, packet.m_nBodySize - 2);
                    if(audio_pre_pts_ == -1){
                        audio_pre_pts_= packet.m_nTimeStamp;
                        if(!packet.m_hasAbsTimestamp) {
                            LogWarn("no init video pts");
                        }
                    }
                    else {
                        if(packet.m_hasAbsTimestamp)
                            audio_pre_pts_= packet.m_nTimeStamp;
                        else
                            audio_pre_pts_ += packet.m_nTimeStamp;
                    }
                    aud_raw->pts = audio_pre_pts_;
                    audio_callable_object_(RTMP_BODY_AUD_RAW, aud_raw, false);
                }
                //                LogInfo("aud finish  t:%ld\n", TimesUtil::GetTimeMillisecond() - cur_time);
            }
            else if (packet.m_packetType == RTMP_PACKET_TYPE_INFO)
            {
                LogInfo("onReadVideoAndAudioInfo ");
                parseScriptTag(packet);
                if(video_width > 0 && video_height>0)
                {
                    FLVMetadataMsg *metadata = new FLVMetadataMsg();
                    metadata->width = video_width;//720;
                    metadata->height = video_height;//480;
                    video_callable_object_(RTMP_BODY_METADATA, metadata, false);
                }
            }
            else
            {
                LogInfo("can't handle it ");
                RTMP_ClientPacket(rtmp_, &packet);
            }
            //            RTMP_ClientPacket(m_pRtmp, &packet);
            RTMPPacket_Free(&packet);

            memset(&packet,0,sizeof(RTMPPacket));
        }
    }
    LogInfo("thread exit");
    return NULL;
}


}

