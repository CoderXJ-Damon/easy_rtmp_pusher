# Easy RTMP Pusher - PTS生成、流转和同步机制详解

**文档更新时间**：2026-03-31  
**涉及版本**：基于 commit 453bc73 (fix)

---

## 📖 目录

- [PTS基础概念](#pts基础概念)
- [PTS生成机制](#pts生成机制)
- [PTS计算流程](#pts计算流程)
- [PTS初始化和配置](#pts初始化和配置)
- [PTS流转全过程](#pts流转全过程)
- [音视频同步实现](#音视频同步实现)
- [误差修复机制](#误差修复机制)
- [调试和日志](#调试和日志)
- [常见问题](#常见问题)

---

## PTS基础概念

### 什么是PTS？

**PTS** (Presentation Time Stamp) 是 **表现时间戳**，表示媒体帧在播放时的显示时刻。

- **单位**：毫秒 (ms)
- **作用**：
  - 音视频同步的基准
  - 媒体播放进度控制
  - 网络传输中的时间参考

### PTS在RTMP协议中的作用

```
RTMP包结构:
┌──────────────────────────────────┐
│     RTMP Header (12 bytes)        │
├──────────────────────────────────┤
│  Timestamp (3 bytes) ← PTS值的来源
│  Message Length (3 bytes)
│  Message Type (1 byte)
│  Stream ID (4 bytes)
├──────────────────────────────────┤
│     RTMP Payload                 │
│  ├─ 音频数据或视频数据
│  └─ 配置信息
└──────────────────────────────────┘
```

---

## PTS生成机制

### 核心类：AVPublishTime（单例模式）

**文件位置**：`avtimebase.h` (第14-187行)

```cpp
class AVPublishTime {
    // 单例模式
    static AVPublishTime* GetInstance() {
        if (s_publish_time == NULL)
            s_publish_time = new AVPublishTime();
        return s_publish_time;
    }
    
    // 核心方法
    uint32_t get_audio_pts();  // 获取音频PTS
    uint32_t get_video_pts();  // 获取视频PTS
};
```

### 两种PTS生成策略

#### 1. **PTS_RECTIFY**（推荐 - 本项目使用）

**特点**：保持规律的帧间隔，平滑PTS增长

```cpp
// avtimebase.h:18
typedef enum PTS_STRATEGY {
    PTS_RECTIFY = 0,    // 矫正模式 ← 默认
    PTS_REAL_TIME       // 实时模式
} PTS_STRATEGY;
```

**工作原理**：
- 计算目标PTS与实际系统时间的误差
- 误差 < 半帧时间 → 按帧间隔递推（保持规律）
- 误差 ≥ 半帧时间 → 重新同步（修复大抖动）

**优势**：
- ✅ 保持恒定帧间隔，避免抖动
- ✅ 更易于网络传输和播放器处理
- ✅ 对轻微的系统时钟抖动有很好的容错性

#### 2. **PTS_REAL_TIME**（实时模式）

**特点**：严格按照系统时间

**使用场景**：特殊应用，对实时性要求极高

---

### PTS时间参数配置

| 参数 | 音频 | 视频 | 说明 |
|------|------|------|------|
| **帧间隔** | 21.3333ms | 40ms | 1000/采样率×样本数 或 1000/帧率 |
| **半帧阈值** | 10.67ms | 20ms | 用于误差判断的阈值 |
| **默认采样率** | 48000Hz | 25fps | 配置中可修改 |
| **每包样本数** | 1024 | 1帧 | AAC编码的标准单位 |

**计算公式**：
```
音频帧间隔 = 1000ms / 采样率 × 每包样本数
           = 1000 / 48000 × 1024
           = 21.3333ms

视频帧间隔 = 1000ms / 帧率
           = 1000 / 25
           = 40ms

半帧阈值 = 帧间隔 / 2
```

---

## PTS计算流程

### 核心算法：以音频为例

**代码位置**：`avtimebase.h` (第44-62行)

```cpp
uint32_t get_audio_pts() {
    // ===== Step 1: 计算相对时间 =====
    // 获取当前系统时间与推流开始时刻的差值（相对时间）
    int64_t pts = getCurrentTimeMsec() - start_time_;
    
    if(PTS_RECTIFY == audio_pts_strategy_) {
        // ===== Step 2: 计算误差 =====
        // 目标PTS = 上一帧PTS + 帧间隔
        // diff = |实际时间 - 目标时间|
        uint32_t diff = (uint32_t)abs(pts - (long long)(audio_pre_pts_ + audio_frame_duration_));
        
        if(diff < audio_frame_threshold_) {
            // ===== Step 3a: 误差小 → 保持规律 =====
            // 误差在半帧以内，按帧间隔累加
            audio_pre_pts_ += audio_frame_duration_;
            
            LogDebug("get_audio_pts1:%u RECTIFY:%0.0lf", diff, audio_pre_pts_);
            return (uint32_t)(((int64_t)audio_pre_pts_) % 0xffffffff);
        }
        
        // ===== Step 3b: 误差大 → 重新同步 =====
        // 误差超过半帧，重新调整基准到实际时间
        audio_pre_pts_ = (double)pts;
        
        LogDebug("get_audio_pts2:%u, RECTIFY:%0.0lf", diff, audio_pre_pts_);
        return (uint32_t)(pts % 0xffffffff);
    }
    else {
        // ===== 实时模式 =====
        audio_pre_pts_ = (double)pts;
        LogDebug("get_audio_pts REAL_TIME:%0.0lf", audio_pre_pts_);
        return (uint32_t)(pts % 0xffffffff);
    }
}
```

### 算法流程图

```
每次调用 get_audio_pts() 时:

┌─────────────────────────────────────┐
│  获取当前系统时间                    │
│  pts = getCurrentTimeMsec()          │
│      - start_time_                  │
└──────────────┬──────────────────────┘
               │
               ▼
┌─────────────────────────────────────┐
│  计算误差                            │
│  diff = |pts - (audio_pre_pts_      │
│              + audio_frame_duration_)│
└──────────────┬──────────────────────┘
               │
               ▼
        ┌──────────────┐
        │  diff < 10.67ms?  │
        └──┬──────────┬──┘
           │          │
       YES │          │ NO
           ▼          ▼
      ┌────────┐  ┌──────────────┐
      │保持规律│  │重新同步      │
      │累加PTS │  │更新基准      │
      │+21.33ms│  │pts→pre_pts_  │
      └───┬────┘  └──────┬───────┘
          │               │
          │    ┌──────────┘
          │    │
          └────┴──→ 返回PTS值
```

### 示意化执行示例

```
推流开始时刻: T0 = getCurrentTimeMsec()
初始值: audio_pre_pts_ = 0

┌─────────────────────────────────────────────────────────────┐
│               系统时间演进与PTS生成过程                       │
└─────────────────────────────────────────────────────────────┘

时刻      系统相对时间  目标PTS    实际diff   判定      返回PTS
────      ──────────  ────────   ────────  ──────    ────────
T0+10ms        10ms     0ms        10ms    <10.67 ✅  0ms (累加)
T0+31ms        31ms     21ms       10ms    <10.67 ✅  21ms
T0+52ms        52ms     42ms       10ms    <10.67 ✅  42ms
T0+73ms        73ms     63ms       10ms    <10.67 ✅  63ms
T0+105ms      105ms    84ms+21=105 0ms     <10.67 ✅  105ms
T0+152ms      152ms    126ms+21=147 5ms    <10.67 ✅  147ms

异常场景 (系统时钟抖动):
────────────────────────
T0+200ms      200ms   189ms+21=210 10ms   <10.67 ✅  210ms  (吸收)
T0+235ms      235ms   210ms+21=231 4ms    <10.67 ✅  231ms
T0+260ms      260ms   231ms+21=252 8ms    <10.67 ✅  252ms
T0+280ms      280ms   252ms+21=273 7ms    <10.67 ✅  273ms
T0+330ms      330ms   273ms+21=294 36ms   ≥10.67 ❌  330ms  (重新同步!)
T0+352ms      352ms   330ms+21=351 1ms    <10.67 ✅  351ms
```

---

## PTS初始化和配置

### 初始化入口

**文件位置**：`pushwork.cpp` (第45-253行)

在 `PushWork::Init()` 中进行PTS系统初始化：

```cpp
// Step 1: 重置推流时间戳基准
AVPublishTime::GetInstance()->Rest();
// 内部执行: start_time_ = getCurrentTimeMsec()
```

### 音频PTS配置

```cpp
// pushwork.cpp:205-208
double audio_frame_duration = 1000.0 / audio_encoder_->get_sample_rate() 
                            * audio_encoder_->GetFrameSampleSize();
// 计算: 1000.0 / 48000 * 1024 ≈ 21.3333ms

LogInfo("audio_frame_duration:%lf", audio_frame_duration);

AVPublishTime::GetInstance()->set_audio_frame_duration(audio_frame_duration);
// 内部: audio_frame_threshold_ = 21.3333 / 2 ≈ 10.67ms

AVPublishTime::GetInstance()->set_audio_pts_strategy(AVPublishTime::PTS_RECTIFY);
// 启用矫正模式
```

### 视频PTS配置

```cpp
// pushwork.cpp:229-231
double video_frame_duration = 1000.0 / video_encoder_->get_framerate();
// 计算: 1000.0 / 25 = 40ms

LogInfo("video_frame_duration:%lf", video_frame_duration);

AVPublishTime::GetInstance()->set_video_pts_strategy(AVPublishTime::PTS_RECTIFY);
// 启用矫正模式
```

### 配置参数总览

```cpp
// 初始化后的全局状态
AVPublishTime instance:
├─ start_time_                    = 当前系统毫秒时间
├─ audio_frame_duration_          = 21.3333ms
├─ audio_frame_threshold_         = 10.67ms
├─ audio_pre_pts_                 = 0ms (第一次返回时设置)
├─ audio_pts_strategy_            = PTS_RECTIFY
├─ video_frame_duration_          = 40ms
├─ video_frame_threshold_         = 20ms
├─ video_pre_pts_                 = 0ms (第一次返回时设置)
└─ video_pts_strategy_            = PTS_RECTIFY
```

---

## PTS流转全过程

### 完整数据流图

```
┌──────────────────────────────────────────────────────────────────┐
│                   音频PTS流转过程（完整路径）                     │
└──────────────────────────────────────────────────────────────────┘

[1] 采集阶段
    │
    ├─ AudioCapturer 线程持续采集
    └─ PCM数据 (48kHz, 每包1024样本)
       └─ 触发 PcmCallback()

[2] 编码和PTS获取阶段
    │
    ├─ AudioResampler::Resample()
    │  └─ S16le (交错) → FLTP (棋盘格式)
    │
    ├─ AACEncoder::Encode()
    │  └─ FLTP PCM → AAC编码数据
    │
    └─ 构建 AudioRawMsg
       │
       ├─ 分配内存: data = new char[size]
       │
       ├─ ⭐ 获取PTS (核心步骤)
       │  │
       │  └─ aud_raw_msg->pts 
       │        = AVPublishTime::GetInstance()->get_audio_pts()
       │        = 调用前文的PTS计算算法
       │
       └─ 设置消息属性
          └─ aud_raw_msg->data[0] = 0xaf  (RTMP音频标记)
             aud_raw_msg->data[1] = 0x01  (原始AAC数据)
             aud_raw_msg->size = aac_size + 2

[3] 入队阶段
    │
    └─ rtmp_pusher->Post(RTMP_BODY_AUD_RAW, aud_raw_msg)
       │
       └─ RTMPPusher::Post()
          │
          └─ Looper::Post()
             │
             └─ NaluLoop::addmsg(msg, false)
                │
                ├─ queue_mutex_.lock()
                │
                ├─ 检查队列大小
                │  ├─ 队列未满 → 直接入队
                │  └─ 队列满   → 智能丢帧 (保留I帧)
                │
                ├─ msg_queue_.push_back(msg)
                │  ├─ what = RTMP_BODY_AUD_RAW
                │  └─ obj = (MsgBaseObj*)aud_raw_msg
                │
                ├─ queue_mutex_.unlock()
                │
                └─ head_data_available_->post()  (信号量通知)

[4] 出队和处理阶段
    │
    └─ RTMPPusher线程 (Looper::loop())
       │
       ├─ 等待队列信号 (head_data_available_->wait())
       │
       ├─ queue_mutex_.lock()
       │
       ├─ msg = msg_queue_.front()
       │  msg_queue_.pop_front()
       │
       ├─ queue_mutex_.unlock()
       │
       └─ RTMPPusher::handle(RTMP_BODY_AUD_RAW, aud_raw_msg)
          │
          ├─ AudioRawMsg *audio_raw = (AudioRawMsg*)data
          │
          ├─ ⭐ 使用PTS值
          │  │
          │  └─ sendPacket(RTMP_PACKET_TYPE_AUDIO,
          │              audio_raw->data,
          │              audio_raw->size,
          │              audio_raw->pts)    ← PTS在这里被使用

[5] RTMP网络发送
    │
    └─ RTMPPusher::sendPacket()
       │
       ├─ RTMPPacket packet
       │
       ├─ 设置包头信息
       │  ├─ packet.m_packetType = RTMP_PACKET_TYPE_AUDIO
       │  ├─ packet.m_nChannel = RTMP_AUDIO_CHANNEL
       │  ├─ packet.m_nTimeStamp = timestamp  (⭐ PTS值)
       │  └─ packet.m_nBodySize = size
       │
       ├─ 复制音频数据
       │  └─ memcpy(packet.m_body, data, size)
       │
       └─ 发送到服务器
          └─ RTMP_SendPacket(rtmp_, &packet, 0)
             └─ 通过网络发送给RTMP服务器


┌──────────────────────────────────────────────────────────────────┐
│                   视频PTS流转过程（完整路径）                     │
└──────────────────────────────────────────────────────────────────┘

[1] 采集阶段
    └─ VideoCapturer → YUV420p数据 → YuvCallback()

[2] 编码和PTS获取
    │
    ├─ H264Encoder::Encode()
    │  └─ YUV → H.264 NALU单元
    │
    └─ 构建 NaluStruct
       │
       ├─ 分配内存并复制数据
       │
       ├─ ⭐ 获取PTS
       │  │
       │  └─ nalu->pts = AVPublishTime::GetInstance()->get_video_pts()
       │
       └─ 设置NALU类型
          └─ nalu->type = video_nalu_buf[0] & 0x1f
             (5=I帧, 1=P帧, etc.)

[3] 入队 → [同音频]

[4] 出队处理
    │
    └─ RTMPPusher::handle(RTMP_BODY_VID_RAW, nalu_struct)
       │
       ├─ NaluStruct *nalu = (NaluStruct*)data
       │
       └─ sendH264Packet((char*)nalu->data,
                        nalu->size,
                        (nalu->type == 0x05),  // I帧判断
                        nalu->pts)             // ⭐ PTS使用

[5] RTMP网络发送
    │
    └─ sendPacket(RTMP_PACKET_TYPE_VIDEO, ...)
       │
       └─ packet.m_nTimeStamp = timestamp  (⭐ PTS值)
```

---

## 音视频同步实现

### 同步核心原理

**音视频同步的本质**：在统一的时间坐标系上，使用相同的时间基准 (`start_time_`)，分别按照各自的帧间隔生成PTS。

#### 统一时间基准

```cpp
// 所有音视频数据都基于同一个起始时刻计时
int64_t start_time_ = getCurrentTimeMsec();  // 推流开始时刻

// 音频PTS计算
int64_t audio_pts = getCurrentTimeMsec() - start_time_;  // 相对时间

// 视频PTS计算
int64_t video_pts = getCurrentTimeMsec() - start_time_;  // 相同的基准!
```

### 同步过程详细解析

```
┌──────────────────────────────────────────────────────────────────┐
│            音视频同步的完整工作流程（PTS_RECTIFY模式）            │
└──────────────────────────────────────────────────────────────────┘

推流启动:
  │
  └─ Rest() → start_time_ = T0 (当前系统毫秒时间)
     
     所有后续PTS都基于T0计算相对时间


音频部分 (采样率48kHz, 帧间隔 ~21.33ms):

系统绝对时间:   T0      T0+21    T0+42    T0+63    T0+84   T0+105  
               ├─────────────────────────────────────────────────
采集和编码:    A1      A2       A3       A4       A5      A6
               │       │        │        │        │       │
PTS_RECTIFY:   │       │        │        │        │       │
               ↓       ↓        ↓        ↓        ↓       ↓
计算的PTS:     0ms    21ms    42ms    63ms    84ms   105ms
累加规律:      +21ms  +21ms   +21ms   +21ms   +21ms
误差检测:      0ms<10.67✅ 1ms<10.67✅ 0ms<10.67✅ ...
结论:          保持规律采样间隔 → PTS = 0, 21, 42, 63, 84, 105 ms

分析:
├─ 虽然实际采集时刻受操作系统调度影响，可能略有抖动
├─ 但由于 diff < 10.67ms (半帧阈值)，被自动吸收
└─ 最终PTS保持恒定21.33ms的递推间隔


视频部分 (帧率25fps, 帧间隔 40ms):

系统绝对时间:   T0      T0+40    T0+80   T0+120   T0+160  T0+200
               ├────────────────────────────────────────────────
采集和编码:    V1      V2       V3      V4       V5      V6
               │       │        │       │        │       │
PTS_RECTIFY:   │       │        │       │        │       │
               ↓       ↓        ↓       ↓        ↓       ↓
计算的PTS:     0ms    40ms    80ms   120ms   160ms  200ms
累加规律:      +40ms  +40ms   +40ms  +40ms   +40ms
误差检测:      0ms<20✅ 0ms<20✅ 0ms<20✅ ...
结论:          保持规律采样间隔 → PTS = 0, 40, 80, 120, 160, 200 ms

分析:
├─ 视频帧率低，帧间隔大，更容易保持规律
└─ 自动机制能更好地平滑时钟抖动


音视频在公共时间轴上的分布:

  0    21    42    63    84    105   126   147   168   189   210 (ms)
  │    │     │     │     │     │     │     │     │     │     │
  A1   A2    A3    A4    A5    A6    A7    A8    A9    A10   A11  (音频)
  V1        V2               V3              V4              V5   (视频)
  │                          │                               │
  └──────────────────────────┴───────────────────────────────┘
         播放器通过PTS对齐，同时播放音频和对应时刻的视频帧
         例如: 播放A1-A6 (0-105ms) 时，对应播放 V1-V2


音视频帧率比例分析:

音频帧率 ≈ 46.875 fps  (1000ms / 21.33ms)
视频帧率 = 25 fps
比例 = 46.875 / 25 = 1.875 : 1

含义: 每1个视频帧对应约1.88个音频帧
      即: 视频帧 V1 → 音频帧 A1, A2
         视频帧 V2 → 音频帧 A3, A4
         ...
```

### PTS同步的三个关键要素

| 要素 | 实现机制 | 效果 |
|------|---------|------|
| **统一时间基准** | 音视频都使用相同的 `start_time_` | 确保音视频在同一坐标系上 |
| **帧间隔递推** | 音频累加21.33ms，视频累加40ms | 保持恒定采样间隔，避免时钟漂移 |
| **自适应矫正** | 误差检测+阈值判断 | 吸收小抖动(< 半帧)，修复大偏差 |

### 播放器如何使用这些PTS值

```
播放器接收到音视频数据流:

音频包序列:  PTS=0,  21,  42,  63,  84, 105, 126, ...
视频包序列:  PTS=0,  40,  80, 120, 160, 200, ...

播放器的同步逻辑:
├─ 维护一条公共的"播放时间轴" (从0开始递增)
│
├─ 当播放时间推进到 0ms 时:
│  ├─ 播放音频包 (PTS=0)
│  └─ 播放视频包 (PTS=0)
│
├─ 当播放时间推进到 40ms 时:
│  ├─ 播放音频包 (PTS=42) [最接近的]
│  └─ 播放视频包 (PTS=40)
│
├─ 当播放时间推进到 80ms 时:
│  ├─ 播放音频包 (PTS=84) [最接近的]
│  └─ 播放视频包 (PTS=80)
│
└─ ... 依此类推，保证音视频步调一致

结果: 最终用户看到的是同步的音视频!
```

---

## 误差修复机制

### 问题：为什么需要误差修复？

在真实运行环境中，存在以下问题：

```
1. 系统调度抖动
   └─ 线程采集时刻受操作系统调度影响
      预期: T+21ms    实际: T+19ms 或 T+23ms (±2ms)

2. 编码耗时变化
   └─ H.264/AAC编码耗时不固定
      有时快: 5ms    有时慢: 8ms

3. 时钟漂移
   └─ 系统时钟与实际时间有微小偏差
      长时间运行可能累积

4. 网络波动
   └─ RTMP推送可能堵塞，影响采集线程
```

### 自适应修复算法

```cpp
// avtimebase.h:46-64 - PTS_RECTIFY模式实现

uint32_t get_audio_pts() {
    int64_t pts = getCurrentTimeMsec() - start_time_;
    
    // 计算实际时间与理想PTS的偏差
    uint32_t diff = abs(pts - (audio_pre_pts_ + audio_frame_duration_));
    
    if(diff < audio_frame_threshold_) {
        // ┌─ 场景A: 小误差 (< 10.67ms)
        // │  
        // │  含义: 虽然实际采集时刻与理想时刻有偏差，
        // │       但偏差很小，在可接受范围内
        // │
        // │  处理: 忽略这个小偏差，继续按帧间隔递推
        // │       这样做的好处是保持PTS的规律性
        // │
        // └─ 结果: PTS保持恒定间隔
        
        audio_pre_pts_ += audio_frame_duration_;
        LogDebug("get_audio_pts1:%u RECTIFY", diff);  // 规律模式日志
        return (uint32_t)(((int64_t)audio_pre_pts_) % 0xffffffff);
    }
    
    // ┌─ 场景B: 大误差 (>= 10.67ms)
    // │
    // │  含义: 实际时刻与理想时刻相差太大
    // │       可能发生了: 系统卡顿、编码延迟突增、等异常
    // │
    // │  处理: 强制重新同步，使用当前实际系统时间作为新基准
    // │       这样做的好处是快速修复大的偏差
    // │
    // └─ 结果: PTS值重新回到实际时间，丢弃了历史累加值
    
    audio_pre_pts_ = (double)pts;
    LogDebug("get_audio_pts2:%u RECTIFY", diff);  // 修复模式日志
    return (uint32_t)(pts % 0xffffffff);
}
```

### 修复效果演示

```
场景1: 正常情况 (小抖动自动吸收)

预期PTS轨迹:     ──────────────────────────
               /  /  /  /  /  /  / (间隔21ms)

实际采集时刻:    ●  ●  ●  ●  ●  ●ᐩ
               (略有偏差，最多±5ms)

最终生成PTS:     ○──○──○──○──○──○
               0 21 42 63 84 105 (完全规律!)
               
解释: 虽然实际采集有抖动，但由于diff < 10.67ms，
      被自动吸收，最终PTS保持恒定21.33ms间隔


场景2: 异常情况 (大偏差强制修复)

预期PTS轨迹:     ──────────┐ ┌──────────
                         /│ │\
实际采集时刻:    ●  ●  ●  ●(卡)●  ●  ●
               时刻:0  21  42  63  105 126

系统卡顿 60ms (第4帧延迟):
│ 第1-3帧: diff = 0, 1, 2ms < 10.67 ✅ → 保持规律
│ 第4帧: 实际时刻=105ms, 理想=(63+21)=84ms
│         diff = |105-84| = 21ms ≥ 10.67 ❌
│         → 强制重新同步到105ms
│
生成的PTS: 0, 21, 42, 63, 105, 126, ...
                              ↑
                           (跳跃，修复卡顿)


场景3: 时钟漂移场景 (长期修正)

前100ms: 系统时钟快于实际 (积累+5ms误差)
后100ms: 系统时钟慢于实际 (积累-5ms误差)

PTS_RECTIFY处理:
├─ 前100ms: 虽然每帧差1-2ms，但累加被吸收
├─ 跨界处: 误差超过阈值时，自动调整基准
└─ 后100ms: 重新对齐到实际时间

结果: 整体漂移被修正，不会造成长期累积偏差
```

### 日志输出解读

```cpp
// avtimebase.h:51, 57, 61
LogDebug("get_audio_pts1:%u RECTIFY:%0.0lf", diff, audio_pre_pts_);
// 含义: diff=误差, RECTIFY=规律模式, audio_pre_pts_=累加的PTS值
// 例: "get_audio_pts1:2 RECTIFY:42.000000" 
//     → 误差2ms, 保持规律, 返回PTS=42

LogDebug("get_audio_pts2:%u RECTIFY:%0.0lf", diff, audio_pre_pts_);
// 含义: diff=误差, RECTIFY=修复模式, audio_pre_pts_=重新同步的PTS值
// 例: "get_audio_pts2:21 RECTIFY:105.000000"
//     → 误差21ms, 执行同步, 返回PTS=105
```

---

## 调试和日志

### 关键时间戳标签

项目在 `avtimebase.h` 中定义了多个调试标签，用于追踪关键时刻：

| 标签 | 含义 | 位置 |
|------|------|------|
| `keytime:metadata` | 发送FLV元数据时刻 | rtmppusher.cpp:86 |
| `keytime:aacheader` | 发送AAC配置时刻 | rtmppusher.cpp:102 |
| `keytime:aacdata` | 发送AAC数据时刻 | rtmppusher.cpp:138 |
| `keytime:avcheader` | 发送H.264配置时刻 | rtmppusher.cpp:102 |
| `keytime:avcframe` | 发送H.264数据时刻 | rtmppusher.cpp:117 |

### 调试日志调用

```cpp
// rtmppusher.cpp:86-87
if(!is_first_metadata_) {
    is_first_metadata_ = true;
    LogInfo("%s:t%u", 
            AVPublishTime::GetInstance()->getMetadataTag(),
            AVPublishTime::GetInstance()->getCurrenTime());
}

// 输出示例:
// "keytime:metadata:t0"
// "keytime:aacheader:t2"
// "keytime:avcheader:t8"
// "keytime:aacdata:t21"
// "keytime:avcframe:t40"
```

### 调试建议

```cpp
// 1. 在PcmCallback和YuvCallback中添加日志
LogInfo("PcmCallback: pts=%u, size=%d", aud_raw_msg->pts, aac_size);
LogInfo("YuvCallback: pts=%u, size=%d", nalu->pts, video_nalu_size_);

// 2. 在get_audio_pts/get_video_pts中追踪误差
LogDebug("audio: diff=%d, strategy=%d, pre_pts=%.0lf",
         diff, audio_pts_strategy_, audio_pre_pts_);

// 3. 比对音视频PTS序列
// 在日志中保存所有PTS值，绘制时间轴图表
// 检查:
//   ├─ 是否保持规律间隔
//   ├─ 是否出现跳跃 (修复事件)
//   └─ 音视频相位差是否合理

// 4. 长期运行监测
// 观察:
//   ├─ PTS是否出现溢出 (% 0xffffffff)
//   ├─ 误差修复频次
//   └─ 是否出现异常大的PTS间隔
```

---

## 常见问题

### Q1: 为什么使用 `% 0xffffffff` 对PTS取余？

**A**: RTMP协议使用32位整数表示时间戳，会自动溢出。

```cpp
// avtimebase.h:54, 58, 62, 78, 82
return (uint32_t)(((int64_t)audio_pre_pts_) % 0xffffffff);
//                                            ↑
//                                   0xffffffff = 4294967295
//                                   max uint32_t值

// 示例:
// 当推流运行很久，PTS累计到 4500000000ms 时
// 4500000000 % 4294967295 = 205032705
// 自动回绕，避免溢出
```

### Q2: PTS_RECTIFY和PTS_REAL_TIME如何选择？

**A**:

| 模式 | 优点 | 缺点 | 适用场景 |
|------|------|------|---------|
| **PTS_RECTIFY** | 保持规律，易处理 | 实时性略降低 | ✅ **推荐使用** |
| **PTS_REAL_TIME** | 完全实时 | 可能有抖动 | 特殊场景 |

```cpp
// 如需切换:
AVPublishTime::GetInstance()->set_audio_pts_strategy(
    AVPublishTime::PTS_REAL_TIME  // 改为实时模式
);
```

### Q3: 为什么音频帧间隔是21.33ms？

**A**: 由AAC编码标准定义。

```
音频采样率: 48000 Hz (每秒48000个采样)
每个AAC帧: 1024个采样
帧间隔 = 1024 / 48000 * 1000 = 21.3333ms

可调整参数:
├─ 修改编码器采样率 (AudioEncoder配置)
├─ 修改每包样本数 (编码标准规定，不建议改)
└─ 则帧间隔会自动重算
```

### Q4: 为什么视频帧间隔是40ms？

**A**: 由视频帧率定义。

```
视频帧率: 25 fps (推流配置)
帧间隔 = 1000 / 25 = 40ms

可调整参数:
└─ 修改video_fps配置参数
   然后video_frame_duration会自动重算
```

### Q5: 如何检查音视频是否同步？

**A**: 比对PTS序列中的位置关系。

```
正常同步的表现:
├─ 两条PTS序列都保持恒定间隔
├─ 音频更频繁 (每21.33ms一个)
├─ 视频较稀疏 (每40ms一个)
├─ 长期看，音频与视频的相位关系稳定
└─ 播放器能流畅播放，无声画不同步

异常表现:
├─ PTS间隔不规律 (多次修复事件)
├─ 音视频PTS相位关系变化
├─ 播放时音频突然提前/延后
└─ 出现跳帧或音频卡顿

调试方法:
└─ 启用DEBUG日志, 导出PTS序列
   然后用Python/Excel绘制时间轴图表分析
```

### Q6: 推流过程中发生系统卡顿，PTS会怎样？

**A**: 取决于卡顿持续时间。

```
场景A: 卡顿 5ms (< 10.67ms)
└─ 被误差修复机制吸收，PTS保持规律
   用户: 无感知

场景B: 卡顿 20ms (>= 10.67ms)
└─ 触发PTS重新同步，出现PTS跳跃
   用户: 可能听到音频轻微卡顿

场景C: 卡顿 100ms
└─ PTS大幅跳跃，明显的音画不同步
   用户: 明显感到卡顿
```

### Q7: 能否手动调整PTS阈值？

**A**: 可以，但需谨慎。

```cpp
// avtimebase.h:176-178
double audio_frame_duration_ = 21.3333;
uint32_t audio_frame_threshold_ = (uint32_t)(audio_frame_duration_/2);
//                                           ↑ 自动计算为半帧时间

// 如需手动调整:
AVPublishTime::GetInstance()->set_audio_frame_duration(21.3333);
// 内部自动设置: threshold_ = 21.3333 / 2 = 10.67ms

// ⚠️  一般不需要修改，除非有特殊需求
```

### Q8: PTS会影响网络传输吗？

**A**: 不会直接影响，但影响播放体验。

```
RTMP包结构:
├─ RTMP Header (固定)
│  └─ m_nTimeStamp = PTS值
├─ RTMP Payload (音视频数据)
└─ 网络发送

PTS的作用:
├─ 告诉RTMP服务器: 这个包在何时播放
├─ 服务器记录但不改变数据
├─ 播放器接收后，按PTS值同步
└─ 网络传输速度不受影响

但是:
└─ 如果PTS不规律，播放器可能:
   ├─ 误判网络延迟
   ├─ 调整缓冲策略
   └─ 可能出现播放卡顿或跳帧
```

---

## 附录：完整数据结构

### AVPublishTime类全部成员

```cpp
class AVPublishTime {
    // ===== 配置方法 =====
    void Rest();  // 重置start_time_
    void set_audio_frame_duration(double frame_duration);
    void set_video_frame_duration(double frame_duration);
    void set_audio_pts_strategy(PTS_STRATEGY pts_strategy);
    void set_video_pts_strategy(PTS_STRATEGY pts_strategy);
    
    // ===== 获取PTS值 =====
    uint32_t get_audio_pts();   // 获取音频PTS
    uint32_t get_video_pts();   // 获取视频PTS
    uint32_t getCurrenTime();   // 获取当前相对时间
    
    // ===== 调试标签 =====
    const char *getKeyTimeTag();        // "keytime"
    const char *getRtmpTag();           // "keytime:rtmp_publish"
    const char *getMetadataTag();       // "keytime:metadata"
    const char *getAacHeaderTag();      // "keytime:aacheader"
    const char *getAacDataTag();        // "keytime:aacdata"
    const char *getAvcHeaderTag();      // "keytime:avcheader"
    const char *getAvcIFrameTag();      // "keytime:avciframe"
    const char *getAvcFrameTag();       // "keytime:avcframe"
    
private:
    // ===== 时间基准 =====
    int64_t start_time_ = 0;  // 推流开始时的系统毫秒时间
    
    // ===== 音频PTS参数 =====
    PTS_STRATEGY audio_pts_strategy_ = PTS_RECTIFY;
    double audio_frame_duration_ = 21.3333;
    uint32_t audio_frame_threshold_ = 10;
    double audio_pre_pts_ = 0;
    
    // ===== 视频PTS参数 =====
    PTS_STRATEGY video_pts_strategy_ = PTS_RECTIFY;
    double video_frame_duration_ = 40;
    uint32_t video_frame_threshold_ = 20;
    double video_pre_pts_ = 0;
    
    // ===== 单例指针 =====
    static AVPublishTime *s_publish_time;
};
```

### AudioRawMsg和NaluStruct中的PTS

```cpp
class AudioRawMsg : public MsgBaseObj {
public:
    uint32_t pts;              // ⭐ PTS由get_audio_pts()设置
    unsigned char *data;
    int size;
    int with_adts_ = 0;
    int type;
};

class NaluStruct : public MsgBaseObj {
public:
    uint32_t pts;              // ⭐ PTS由get_video_pts()设置
    unsigned char *data;
    int type;
    int size;
};
```

---

## 参考资源

### 相关源文件

| 文件 | 位置 | 主要内容 |
|------|------|---------|
| avtimebase.h | include/ | AVPublishTime核心实现 |
| pushwork.cpp | src/ | PTS初始化 (205-231行) |
| pushwork.cpp | src/ | PTS获取使用 (384, 432行) |
| rtmppusher.cpp | src/ | PTS在RTMP中的应用 (359-380行) |
| mediabase.h | include/ | 数据结构定义 |

### 相关概念

- **RTMP协议**：实时多媒体传输协议
- **H.264/H.265**：视频编码标准
- **AAC**：音频编码标准
- **时间戳(Timestamp)**：媒体同步基础
- **PTS vs DTS**：PTS用于显示，DTS用于解码

---

## 更新记录

| 日期 | 版本 | 更改内容 |
|------|------|---------|
| 2026-03-31 | 1.0 | 初版完成，包括PTS生成、流转、同步和误差修复全套机制 |

---

**文档作者**：Claude Code  
**文档说明**：此文档是对Easy RTMP Pusher项目音视频PTS机制的详细解析，涵盖实现原理、代码位置、算法流程、同步方案和常见问题。





