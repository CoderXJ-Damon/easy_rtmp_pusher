#include "captuerelooper.h"
#include "dlog.h"

namespace LQF
{
void* CaptureLooper::trampoline(void* p) {
    LogInfo("at CaptureLooper trampoline");
    ((CaptureLooper*)p)->Loop();
    return NULL;
}

CaptureLooper::CaptureLooper()
{
    request_exit_ = false;
}

RET_CODE CaptureLooper::Start()
{
    LogInfo("at CaptureLooper create");
    worker_ = new std::thread(trampoline, this);
    if(worker_ == NULL)
    {
        LogError("new std::thread failed");
        return RET_FAIL;
    }

    running_ = true;
    return RET_OK;
}


CaptureLooper::~CaptureLooper()
{
    if (running_)
    {
        LogInfo("CaptureLooper deleted while still running. Some messages will not be processed");
        Stop();
    }
}


void CaptureLooper::Stop()
{
    request_exit_ = true;
    if(worker_)
    {
        worker_->join();
        delete worker_;
        worker_ = NULL;
    }
    running_ = false;
}

}
