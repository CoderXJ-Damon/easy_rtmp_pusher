TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt
#DEFINES += _WIN32
SOURCES += main.cpp \
    librtmp/amf.c \
    librtmp/hashswf.c \
    librtmp/log.c \
    librtmp/parseurl.c \
    librtmp/rtmp.c \
    rtmp_publish.cpp \
    rtmp_pull.cpp \
    aacencoder.cpp \
    audioresample.cpp \
    dlog.cpp \
    utest_aac.cpp \
    rtmppusher.cpp \
    looper.cpp \
    h264encoder.cpp \
    rtmpbase.cpp \
    naluloop.cpp \
    pushwork.cpp \
    audiocapturer.cpp \
    captuerelooper.cpp \
    mediabase.cpp \
    aacrtmppackager.cpp \
    videocapturer.cpp \
    videooutsdl.cpp \
    audiooutsdl.cpp \
    ringbuffer.cpp \
    videodeccodeloop.cpp \
    avsync.cpp
win32 {
INCLUDEPATH += $$PWD/ffmpeg-4.2.1-win32-dev/include
LIBS += $$PWD/ffmpeg-4.2.1-win32-dev/lib/avformat.lib   \
        $$PWD/ffmpeg-4.2.1-win32-dev/lib/avcodec.lib    \
        $$PWD/ffmpeg-4.2.1-win32-dev/lib/avdevice.lib   \
        $$PWD/ffmpeg-4.2.1-win32-dev/lib/avfilter.lib   \
        $$PWD/ffmpeg-4.2.1-win32-dev/lib/avutil.lib     \
        $$PWD/ffmpeg-4.2.1-win32-dev/lib/postproc.lib   \
        $$PWD/ffmpeg-4.2.1-win32-dev/lib/swresample.lib \
        $$PWD/ffmpeg-4.2.1-win32-dev/lib/swscale.lib
INCLUDEPATH += $$PWD/SDL2/include
LIBS += $$PWD/SDL2/lib/x86/SDL2.lib
}

HEADERS += \
    librtmp/amf.h \
    librtmp/bytes.h \
    librtmp/dh.h \
    librtmp/dhgroups.h \
    librtmp/handshake.h \
    librtmp/http.h \
    librtmp/log.h \
    librtmp/rtmp.h \
    librtmp/rtmp_sys.h \
    aacencoder.h \
    audio.h \
    audioresample.h \
    dlog.h \
    rtmppusher.h \
    looper.h \
    semaphore.h \
    h264encoder.h \
    rtmpbase.h \
    mediabase.h \
    naluloop.h \
    pushwork.h \
    audiocapturer.h \
    captuerelooper.h \
    timeutil.h \
    rtmppackager.h \
    aacrtmppackager.h \
    videocapturer.h \
    rtmpplayer.h \
    videooutsdl.h \
    audiooutsdl.h \
    ringbuffer.h \
    avsync.h
