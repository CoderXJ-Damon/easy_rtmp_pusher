#ifndef SEMAPHORE_H
#define SEMAPHORE_H

#include <mutex>
#include <condition_variable>
#include <algorithm>

namespace LQF
{
class Semaphore
{
public:
    struct closed_exception {};
public:
    explicit Semaphore(size_t cnt = 0)
        : count_(cnt)
        , opened_(true)
    {}
    ~Semaphore(){}
    void open()
    {
        std::lock_guard<std::mutex> _(mutex_);
        opened_ = true;
    }
    void close()
    {
        std::lock_guard<std::mutex> _(mutex_);
        opened_ = false;
        event_.notify_all();
    }
    void wait()
    {
        std::unique_lock<std::mutex> lck(mutex_);
        event_.wait(lck, [this]
        {
            if (!opened_)
            {
                throw closed_exception();
            }
            return count_ > 0;
        });
        --count_;
    }
    void post(size_t n = 1)
    {
        std::unique_lock<std::mutex> lck(mutex_);
        count_ += n;
        event_.notify(lck, n);
    }
protected:
    class Guard
    {
    public:
        explicit Guard(size_t& waiters)
            : waiters_(waiters)
        {
            ++waiters_;
        }
        ~Guard()
        {
            --waiters_;
        }
    private:
        size_t & waiters_;
    };
    class Event
    {
    public:
        void wait(std::unique_lock<std::mutex>& lck)
        {
            Guard _(waiters_);
            cnd_.wait(lck);
        }
        template <typename F>
        void wait(std::unique_lock<std::mutex>& lck, F f)
        {
            Guard _(waiters_);
            cnd_.wait(lck, f);
        }
        void notify(std::unique_lock<std::mutex>& lck, size_t n = 1)
        {
            auto times = std::min(n, waiters_);
            for (size_t i = 0; i < times; i++)
            {
                cnd_.notify_one();
            }
        }
        void notify_all()
        {
            cnd_.notify_all();
        }
    private:
        std::condition_variable cnd_;
        size_t                  waiters_{ 0 };
    };
private:
    std::mutex mutex_;
    Event     event_;
    size_t     count_{ 0 };
    bool       opened_{ false };
};
}
#endif // SEMAPHORE_H
