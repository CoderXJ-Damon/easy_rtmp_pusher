# RTMP项目 - 快速参考

## 🎯 项目的核心是什么？
这个项目实现**实时音视频推流**，整个流程：
```
采集数据 → 编码 → RTMP打包 → 上传到直播服务器
```

## 📊 模块依赖关系

```
┌─────────────────────────────────────────────────────────┐
│                      PushWork (主控)                      │
│  负责初始化、协调所有模块、启动推流循环                    │
└──────────┬──────────────────────────┬────────────────────┘
           │                          │
       ┌───▼────────┐            ┌───▼──────────┐
       │   音频链路   │            │   视频链路    │
       └───┬────────┘            └───┬──────────┘
           │                          │
    ┌──────▼──────┐            ┌─────▼──────┐
    │AudioCapturer│            │VideoCapturer│
    │(采集)       │            │(采集)       │
    └──────┬──────┘            └─────┬──────┘
           │                          │
    ┌──────▼──────────┐        ┌─────▼────────┐
    │ AudioResampler  │        │ H264Encoder  │
    │ (重采样格式)    │        │ (编码)       │
    └──────┬──────────┘        └─────┬────────┘
           │                          │
    ┌──────▼──────┐            ┌─────▼──────┐
    │ AACEncoder  │            │ImageScale  │
    │ (AAC编码)   │            │ (缩放)     │
    └──────┬──────┘            └─────┬──────┘
           │                          │
    ┌──────▼──────────────┐           │
    │AACRTMPPackager      │           │
    │(RTMP格式打包)       │           │
    └──────┬──────────────┘           │
           │                          │
           └──────────┬───────────────┘
                      │
           ┌──────────▼───────────┐
           │   RTMPPusher         │
           │ (发送到RTMP服务器)   │
           └──────────┬───────────┘
                      │
           ┌──────────▼──────────────┐
           │  RTMPBase + librtmp    │
           │  (底层RTMP协议实现)    │
           └───────────┬────────────┘
                       │
              ┌────────▼──────────┐
              │  RTMP服务器        │
              │  (直播云)         │
              └─────────────────┘
```

## 🔑 5个关键模块

| 模块 | 文件 | 职责 |
|------|------|------|
| **PushWork** | `pushwork.*` | 全局协调，初始化和启动所有子模块 |
| **AudioCapturer** | `audiocapturer.*` | 采集麦克风/文件中的PCM音频 |
| **H264Encoder** | `h264encoder.*` | 将YUV视频帧编码为H.264格式 |
| **RTMPPusher** | `rtmppusher.*` | 把编码好的音视频发送到RTMP服务器 |
| **RTMPBase** | `rtmpbase.*` | 管理RTMP连接（连接/断开） |

## 💡 快速定位代码

### 想修改音频采样率？
→ `pushwork.cpp` 中搜索 `"mic_sample_rate"`

### 想修改视频分辨率？
→ `pushwork.cpp` 中搜索 `"desktop_width"` 和 `"desktop_height"`

### 想改变RTMP推流地址？
→ `main_publish.cpp` 中修改 `RTMP_URL`

### 想了解H.264编码流程？
→ `h264encoder.cpp` 中的 `Encode()` 方法

### 想了解RTMP推流流程？
→ `rtmppusher.cpp` 中的 `handle()` 方法处理来自编码器的数据

## 🎬 关键数据结构

### NaluStruct - 编码后的媒体数据
```cpp
// 代表一个编码单元（NALU）
struct NaluStruct {
    uint8_t* data;        // 数据指针
    int32_t size;         // 数据大小
    uint64_t timestamp;   // 时间戳
    bool is_keyframe;     // 是否关键帧
};
```

### Properties - 配置参数容器
```cpp
// 用于传递参数，类似Map/Dict
Properties properties;
properties.SetProperty("audio_test", 1);
properties.SetProperty("rtmp_url", "rtmp://...");
```

## 🔗 核心数据流向

### 音频数据流
```
PCM采样
  → AudioCapturer.Loop() 按时间周期调用回调
  → PushWork.PcmCallback() 接收PCM数据
  → AudioResampler 重采样
  → AACEncoder 编码为AAC
  → AACRTMPPackager 打包
  → RTMPPusher 发送
```

### 视频数据流
```
YUV采样
  → VideoCapturer.Loop() 按帧率周期调用回调
  → PushWork.YuvCallback() 接收YUV数据
  → H264Encoder 编码为H.264
  → RTMPPusher 发送 (通过消息队列)
```

## 📌 特殊点

1. **音频直接在PcmCallback中编码** - 快速，不需要队列
2. **视频通过消息队列发送** - 使用NaluLoop的线程安全队列
3. **RTMP发送也在独立线程** - RTMPPusher继承自NaluLoop
4. **时间同步** - 由AVSync和AvPublishTime处理

## 🧪 测试模式

项目支持不读麦克风和摄像头，而是从**本地文件**读取：

```cpp
// 启用音频测试模式
properties.SetProperty("audio_test", 1);
properties.SetProperty("input_pcm_name", "buweishui_48000_2_s16le.pcm");

// 启用视频测试模式
properties.SetProperty("video_test", 1);
properties.SetProperty("input_yuv_name", "720x480_25fps_420p.yuv");
```

这样便于开发调试，不需要真实的摄像头和麦克风。

## 📈 代码阅读路径

### 路径1：快速上手 (30分钟)
1. `main_publish.cpp` - 了解入口
2. `pushwork.h` - 看初始化流程
3. `pushwork.cpp` - 看如何启动子模块

### 路径2：理解音频 (1小时)
1. `audiocapturer.cpp` - 采集逻辑
2. `aacencoder.cpp` - 编码逻辑
3. `aacrtmppackager.cpp` - 打包逻辑

### 路径3：理解视频 (1小时)
1. `videocapturer.cpp` - 采集逻辑
2. `h264encoder.cpp` - 编码逻辑

### 路径4：理解推流 (1小时)
1. `rtmpbase.cpp` - 连接管理
2. `rtmppusher.cpp` - 推流逻辑

---

## 🚨 容易混淆的地方

### 1. 线程问题
- **AudioCapturer** 运行在自己的线程中，定期回调给PushWork
- **VideoCapturer** 运行在自己的线程中，定期回调给PushWork
- **RTMPPusher** 也运行在自己的线程中，通过消息队列接收数据

### 2. 时间戳问题
- PCM数据有时间戳
- YUV帧有时间戳
- RTMP包发送时需要正确的时间戳

### 3. 首帧问题
- RTMP协议要求先发送Metadata（视频/音频信息）
- 然后发送SPS/PPS（视频序列头）
- 然后发送具体的音视频数据

## 🎓 学习顺序建议

1. **第一天**：读 `main_publish.cpp` + `pushwork.h`，理解整体结构
2. **第二天**：读 `audiocapturer.cpp` + `aacencoder.cpp`，理解音频链
3. **第三天**：读 `h264encoder.cpp` + `videocapturer.cpp`，理解视频链
4. **第四天**：读 `rtmppusher.cpp` + `rtmpbase.cpp`，理解推流过程
5. **第五天**：尝试修改参数，编译运行测试

---

**祝你学习顺利！有疑问时，查看对应模块的头文件注释。** 🎉
