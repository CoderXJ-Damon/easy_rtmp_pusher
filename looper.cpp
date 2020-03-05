#include "Looper.h"
#include "dlog.h"
#include "timeutil.h"
namespace LQF
{
void* Looper::trampoline(void* p)
{
    LogInfo("at Looper trampoline");
    ((Looper*)p)->loop();
    return NULL;
}

Looper::Looper() {
    LogInfo("at Looper create");
    head_data_available_ = new Semaphore(0);
    worker_ = new std::thread(trampoline, this);
    running_ = true;
}


Looper::~Looper()
{
    if (running_)
    {
        LogInfo("Looper deleted while still running. Some messages will not be processed");
        Stop();
    }
}

//
void Looper::Post(int what, MsgBaseObj *data, bool flush)
{
    LooperMessage *msg = new LooperMessage();
    msg->what = what;
    msg->obj = data;
    msg->quit = false;
    addmsg(msg, flush);
}

void Looper::addmsg(LooperMessage *msg, bool flush)
{
    // 获取锁，发数据
    int64_t t1 = TimesUtil::GetTimeMillisecond();
    queue_mutex_.lock();
    if (flush) {
        msg_queue_.clear();
    }
    msg_queue_.push_back(msg);
    queue_mutex_.unlock();
    // 发送数据进行通知
    LogInfo("post msg %d, size:%d", msg->what, msg_queue_.size());

    head_data_available_->post();
    //    LogInfo("Looper post");
    int64_t t2 = TimesUtil::GetTimeMillisecond();
    if(t2 - t1 >10)
    {
        LogWarn("t2 - t1 = %ld", t2-t1);
    }
}

void Looper::loop()
{
    LogInfo("into loop");
    LooperMessage *msg;
    while(true)
    {
        queue_mutex_.lock();
        if(msg_queue_.size() > 0)
        {
            msg = msg_queue_.front();
            msg_queue_.pop_front();
            queue_mutex_.unlock();
            //quit 退出
            if (msg->quit)
            {
                break;
            }

            LogDebug("processing msg %d", msg->what);
            handle(msg->what, msg->obj);
            delete msg;
        }
        else
        {
            queue_mutex_.unlock();
            //            LogInfo("no msg");
            head_data_available_->wait();
            LogDebug("wait get msg:%d", msg_queue_.size());
            continue;
        }
        //  std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    delete msg->obj;
    delete msg;
    while(msg_queue_.size() > 0)
    {
        msg = msg_queue_.front();
        msg_queue_.pop_front();
        delete msg->obj;
        delete msg;
    }
}

void Looper::Stop()
{
    if(running_)
    {
        LogInfo("Stop");
        LooperMessage *msg = new LooperMessage();
        msg->what = 0;
        msg->obj = NULL;
        msg->quit = true;
        addmsg(msg, true);
        if(worker_)
        {
            worker_->join();
            delete worker_;
            worker_ = NULL;
        }
        if(head_data_available_)
            delete head_data_available_;

        running_ = false;
    }
}

void Looper::handle(int what, MsgBaseObj* obj)
{
    LogInfo("dropping msg %d %p", what, obj);
}
}
