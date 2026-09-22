#pragma once
#include "channel.h"
#include <sys/timerfd.h>
#include <time.h>
#include <memory>
#include <chrono>
#include <functional>
#include <cstdint>
#include <map>
#include <unordered_map>
#include <cstring>

using Clock = std::chrono::steady_clock;
using TimePoint = Clock::time_point;
using Duration = Clock::duration;  // 通常是 nanoseconds

class EventLoop;

struct Timer{
    std::uint64_t id;
    TimePoint expireTime;           //到期时间
    bool periodic;                  //是否周期执行
    Duration interval;              //周期间隔
    std::function<void()> callback;//回调函数
};

class TimerQueue{
private:
    int timer_fd_;
    EventLoop* loop_;
    std::unique_ptr<Channel> timerChannel_;
    size_t id_count_;//计时器ID计数器
    std::multimap<TimePoint, std::uint64_t> tasks_;
    std::unordered_map<std::uint64_t, Timer> timers_;
public:
    TimerQueue(EventLoop* loop);
    ~TimerQueue();
    int addTimer(Duration delay, std::function<void()> func);
    int addTimer(Duration delay, Duration interval, std::function<void()> func);
    int cancelTimer(std::uint64_t id);
    void readCallback();
    void start(Duration expire_time);
    void stop();
    void check();
};