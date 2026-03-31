# 文件逐一说明

## 核心推流文件

### main_publish.cpp
**功能**：应用程序主入口
**关键代码**：
- 创建 `PushWork` 对象
- 设置推流参数（音视频配置、RTMP地址等）
- 调用 `pushwork.Init()` 初始化
- 调用 `pushwork.Loop()` 启动推流

**关键变量**：
- `RTMP_URL`：推流地址（可在此修改）
- `Properties`：配置容器

---

### pushwork.h / pushwork.cpp
**功能**：推流工作协调器，是整个应用的大脑
**关键职责**：
- 初始化音频采集、编码、打包、推流全链路
- 初始化视频采集、编码、推流全链路
- 管理所有子模块的生命周期
- 接收采集回调数据，转发给编码器

**关键方法**：
- `Init(properties)`：初始化所有模块
- `DeInit()`：清理资源
- `Loop()`：启动推流（阻塞式）
- `AudioCallback(nalu_data)`：接收编码后的AAC数据
- `VideoCallback(nalu_data)`：接收编码后的H.264数据
- `PcmCallback(pcm, size)`：接收采集的PCM数据
- `YuvCallback(yuv, size)`：接收采集的YUV数据

**成员变量**：
- `audio_capturer_`：音频采集器
- `audio_resampler_`：音频重采样器
- `audio_encoder_`：AAC编码器
- `video_capturer`：视频采集器
- `video_encoder_`：H.264编码器
- `rtmp_pusher`：RTMP推流器

---

## 音频处理文件

### audiocapturer.h / audiocapturer.cpp
**功能**：音频采集器，采集麦克风或本地PCM文件
**继承自**：`CommonLooper`（运行在独立线程中）
**工作模式**：
- 实时模式：从系统麦克风采集
- 测试模式：从本地PCM文件读取

**关键方法**：
- `Init(properties)`：初始化
  - "audio_test"：是否启用测试模式
  - "input_pcm_name"：PCM文件路径
  - "sample_rate"：采样率
  - "channels"：通道数
  - "sample_fmt"：采样格式
- `Loop()`：循环采集（运行在线程中）
- `AddCallback(callback)`：设置PCM数据回调

**采集周期**：约21ms（1024个样本 ÷ 48000Hz采样率）

**调用链**：
```
AudioCapturer.Loop()
  → callback_get_pcm_(pcm, size)  // 回调给PushWork
  → PushWork.PcmCallback()
```

---

### audioresample.h / audioresample.cpp
**功能**：音频重采样，转换采样格式和采样率
**使用库**：FFmpeg的libswresample
**场景**：
- 麦克风采样率可能是44.1kHz，编码器需要48kHz
- 采样格式从S16转换为浮点数

**关键方法**：
- `Init(in_fmt, in_rate, out_fmt, out_rate, channels)`：初始化
- `Resample(in, in_samples, out, out_samples)`：重采样

---

### aacencoder.h / aacencoder.cpp
**功能**：AAC音频编码器
**编码流程**：
1. 接收重采样后的PCM数据
2. 编码为AAC格式
3. 生成ADTS头（AAC标准格式）
4. 返回编码后的二进制数据

**关键方法**：
- `Init(properties)`：初始化编码器
  - "sample_rate"：采样率
  - "channels"：通道数
  - "bitrate"：编码比特率
- `Encode(frame, out, out_len)`：编码AVFrame
- `Encode(frame, pts, flush)`：编码并返回AVPacket

**重要信息提取**：
- `GetAdtsHeader()`：获取ADTS头
- `get_sample_rate()`：获取采样率
- `get_channels()`：获取通道数
- `get_bit_rate()`：获取编码码率

**ADTS格式**：音频数据按AAC标准格式打包，包含7字节ADTS头

---

### aacrtmppackager.h / aacrtmppackager.cpp
**功能**：将AAC编码数据打包成RTMP格式
**流程**：
1. 从AACEncoder接收编码数据
2. 生成ADTS头
3. 构造RTMP音频数据包
4. 发送给RTMPPusher

**关键方法**：
- `Package(data, size, timestamp)`：打包RTMP音频数据

---

## 视频处理文件

### videocapturer.h / videocapturer.cpp
**功能**：视频采集器，采集桌面或本地YUV文件
**继承自**：`CommonLooper`（运行在独立线程中）
**工作模式**：
- 实时模式：从屏幕/摄像头采集
- 测试模式：从本地YUV文件读取

**关键方法**：
- `Init(properties)`：初始化
  - "x", "y", "width", "height"：采集区域
  - "format"：像素格式（缺省YUV420P）
  - "fps"：帧率
  - "video_test"：测试模式开关
  - "input_yuv_name"：YUV文件路径
- `Loop()`：循环采集（运行在线程中）
- `AddCallback(callback)`：设置YUV数据回调

**采集周期**：由FPS决定（25fps = 40ms/帧）

---

### h264encoder.h / h264encoder.cpp
**功能**：H.264视频编码器，将YUV视频帧编码为H.264格式
**编码流程**：
1. 接收YUV数据
2. 编码为H.264格式（NALU单元）
3. 提取SPS和PPS信息
4. 标记关键帧

**关键方法**：
- `Init(properties)`：初始化编码器
  - "width", "height"：分辨率
  - "fps"：帧率
  - "bitrate"：编码码率
  - "gop"：GOP大小（关键帧间隔）
  - "b_frames"：B帧数量（直播一般设为0）
- `Encode(yuv, out, out_size)`：编码YUV数据
- `Encode(frame, out, out_size)`：编码AVFrame
- `Encode(yuv, pts, flush)`：编码并返回AVPacket

**SPS/PPS提取**：
- `get_sps()`：获取SPS（序列参数集）
- `get_pps()`：获取PPS（图片参数集）
- 这两项需要在RTMP推流时首先发送给服务器

**关键属性**：
- `get_width()`, `get_height()`：分辨率
- `get_framerate()`：帧率
- `get_bit_rate()`：码率

---

### imagescale.h / imagescale.cpp
**功能**：图像缩放，调整视频分辨率
**使用库**：FFmpeg的libswscale
**应用场景**：
- 采集1920x1080，但需要推送720x480
- 适配不同设备的分辨率

**关键方法**：
- `Init(in_width, in_height, in_fmt, out_width, out_height, out_fmt)`
- `Scale(in, out)`：缩放图像

---

### videooutsdl.h / videooutsdl.cpp
**功能**：使用SDL2库显示推流的视频画面（调试用）
**应用场景**：
- 启用 `rtmp_debug=1` 时显示
- 对比推流延迟和拉流延迟

**关键方法**：
- `Init(width, height, format)`：初始化显示
- `Display(yuv_data, size)`：显示YUV画面

---

## RTMP协议文件

### rtmpbase.h / rtmpbase.cpp
**功能**：RTMP连接管理
**职责**：
- 建立/断开RTMP连接
- 检查连接状态
- 管理RTMP对象生命周期

**关键方法**：
- `Connect(url)`：连接到RTMP服务器
- `Disconnect()`：断开连接
- `IsConnect()`：检查连接状态
- `SetReceiveAudio(bool)`：设置是否接收音频
- `SetReceiveVideo(bool)`：设置是否接收视频

**成员**：
- `rtmp_`：底层RTMP对象（来自librtmp库）
- `url_`：RTMP连接地址

---

### rtmppusher.h / rtmppusher.cpp
**功能**：RTMP推流器，负责发送音视频数据到RTMP服务器
**继承自**：`NaluLoop` 和 `RTMPBase`
**工作流程**：
1. 接收编码后的H.264和AAC数据
2. 构造RTMP包
3. 发送到RTMP服务器

**关键方法**：
- `SendMetadata(metadata)`：发送媒体信息（视频宽高、帧率、音频参数）
- `SendAudioSpecificConfig(data, length)`：发送音频配置
- `handle(what, data)`：消息处理（接收编码数据）

**发送顺序**（重要！）：
1. Metadata（必须首先发送）
2. 视频序列头（SPS/PPS）
3. 音频配置（AudioSpecificConfig）
4. 实际音视频数据

**消息类型**：
- `RTMPPUSHER_MES_H264_DATA`：H.264视频数据
- `RTMPPUSHER_MES_AAC_DATA`：AAC音频数据

**RTMP数据包格式**：
- 音频：RTMP_AUDIO_CHANNEL (4)
- 视频：RTMP_VIDEO_CHANNEL (6)
- 时间戳、长度、数据类型等字段

---

### librtmp/
**功能**：底层RTMP协议实现（第三方库）
**主要文件**：
- `rtmp.h/c`：RTMP核心协议
- `amf.h/c`：AMF数据编解码（用于Metadata）
- `handshake.h/c`：RTMP握手过程
- `bytes.h`：字节操作
- `log.h/c`：日志

**不需要深入**：除非要修改RTMP协议细节

---

## 辅助工具文件

### commonlooper.h / commonlooper.cpp
**功能**：通用循环基类
**特点**：
- 运行在独立线程中
- 定期调用 `Loop()` 虚函数
- AudioCapturer 和 VideoCapturer 都继承自它

---

### looper.h / looper.cpp
**功能**：基础线程循环类
**特点**：
- 更低级的线程管理
- CommonLooper 基于它实现

---

### naluloop.h / naluloop.cpp
**功能**：NALU数据的线程安全队列处理
**特点**：
- 使用RingBuffer存储NALU数据
- H264Encoder 输出给 RTMPPusher 通过它
- RTMPPusher 继承自 NaluLoop

**数据流**：
```
H264Encoder
  → NaluLoop queue (thread-safe)
  → RTMPPusher.handle()
  → RTMPPusher 发送
```

---

### avsync.h / avsync.cpp
**功能**：音视频同步控制
**职责**：
- 确保音频和视频时间戳同步
- 处理音视频不同步的情况

---

### avtimebase.h / avtimebase.cpp
**功能**：时间基准管理
**职责**：
- 管理时间戳的基准
- 确保时间戳的连续性和正确性

---

### avpublishtime.h / avpublishtime.cpp
**功能**：发布时间戳管理
**职责**：
- 计算每帧数据的发布时间戳
- 用于RTMP包的时间戳字段

---

### mediabase.h / mediabase.cpp
**功能**：基础数据结构和属性管理
**关键类**：
- `Properties`：参数容器（类似Map/Dict）
- `RET_CODE`：返回码枚举
- `NaluStruct`：编码单元结构

**NaluStruct定义**：
```cpp
struct NaluStruct {
    uint8_t* data;          // 数据指针
    int32_t size;           // 数据大小
    uint64_t timestamp;     // 时间戳
    bool is_keyframe;       // 是否关键帧
};
```

---

### ringbuffer.h
**功能**：环形缓冲区（线程安全队列）
**用途**：
- 在多线程间传递数据
- AudioCapturer/VideoCapturer 通过它将数据传给编码器
- 减少内存分配，提高效率

**特点**：
- 环形结构，避免频繁重新分配
- 线程安全（使用锁）
- 支持阻塞读写

---

### timeutil.h
**功能**：时间工具函数
**用途**：
- 获取当前时间戳
- 时间转换
- 用于同步和日志

---

### dlog.h / dlog.cpp
**功能**：日志系统
**关键函数**：
- `init_logger(filename, level)`：初始化日志
- `LogInfo(msg)`：信息日志
- `LogError(msg)`：错误日志
- `LogWarn(msg)`：警告日志

**输出**：
- 控制台输出
- 文件输出（rtmp_push.log）

---

### semaphore.h
**功能**：信号量，用于线程同步
**用途**：
- 线程间的同步和通信
- 实现阻塞/非阻塞等待

---

### codecs.h
**功能**：编码器相关常量定义
**包含**：
- 音频编码类型
- 视频编码类型
- 采样格式常量

---

### rtmppackager.h
**功能**：RTMP打包基类（不含实现）
**作用**：
- 定义打包接口
- AACRTMPPackager 继承自它

---

### audio.h
**功能**：音频相关的常量和结构定义
**包含**：
- 采样率常量
- 通道定义
- 音频格式常量

---

## 配置和项目文件

### rtmp-publish.pro
**功能**：Qt 项目配置文件
**包含**：
- 项目类型（应用程序）
- 源文件列表
- 头文件列表
- 第三方库引入
- 编译标志
- 平台特定配置

**第三方库**：
- FFmpeg：音视频编码库
- SDL2：图形显示库
- libws2_32：Windows网络库

---

## 文档文件

### README.md
**内容**：
- 项目简介
- 开发环境搭建指南
- 测试数据准备方法

### PROJECT_GUIDE.md（新添加）
**内容**：
- 详细项目分析
- 模块功能说明
- 数据流向图
- 上手指南
- 配置参数说明

### QUICK_REFERENCE.md（新添加）
**内容**：
- 快速参考
- 模块依赖关系
- 关键数据结构
- 学习路径建议

---

## 文件大小关系（工作量估计）

```
重点文件（必须理解）：
  pushwork.cpp          ✓✓✓ 核心协调
  h264encoder.cpp       ✓✓✓ 关键逻辑
  aacencoder.cpp        ✓✓  音频编码
  rtmppusher.cpp        ✓✓  推流逻辑
  audiocapturer.cpp     ✓   采集逻辑
  videocapturer.cpp     ✓   采集逻辑

重要但可跳过：
  audioresample.cpp      • FFmpeg包装
  aacrtmppackager.cpp    • 简单打包
  rtmpbase.cpp          • 连接管理
  naluloop.cpp          • 消息队列

不用看：
  librtmp/*             ✗ 第三方库
  dlog.cpp              ✗ 日志工具
  videooutsdl.cpp       ✗ 调试显示
  imagescale.cpp        ✗ 可选工具
```

---

**总结**：这个项目共约40个源文件，分为5大模块（推流、音频、视频、RTMP、工具）。理解核心的5个文件就能掌握整个推流流程。🎯
