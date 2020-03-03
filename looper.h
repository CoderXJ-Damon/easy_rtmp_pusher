#ifndef LOOPER_H
#define LOOPER_H
#include <deque>
#include <thread>

#include "mediabase.h"
#include "semaphore.h"


namespace LQF
{

struct LooperMessage;
typedef struct LooperMessage LooperMessage;

// 消息载体
struct LooperMessage {
    int what;
    MsgBaseObj *obj;
    bool quit;
};

class Looper
{
public:
    Looper();
    virtual ~Looper();
    //flush 是否清空消息队列
    void Post(int what, MsgBaseObj *data, bool flush = false);
    void Stop();

    virtual void handle(int what, MsgBaseObj *data);
private:
	virtual void addmsg(LooperMessage *msg, bool flush);
    static void* trampoline(void* p);
    void loop();
protected:
    std::deque< LooperMessage * > msg_queue_;
    std::thread *worker_;
    Semaphore *head_data_available_;
    std::mutex queue_mutex_;
    bool running_;
};

}
#endif // LOOPER_H
