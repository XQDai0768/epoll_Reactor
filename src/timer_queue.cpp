#include "timer_queue.h"
#include <iostream>
#include <vector>

TimerQueue::TimerQueue(EventLoop* loop){

    id_count_ = 0;

    //创建tfd
    timer_fd_ = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);

    if(timer_fd_ == -1){
        perror("timerfd_create:");
    }

    loop_ = loop;

    timerChannel_ = std::make_unique<Channel>(loop_, timer_fd_);

    timerChannel_->setReadCallback([this](){
        readCallback();
    });

    if(timerChannel_->enableReading() == -1){
		std::cout << "Error:enableReading()" << std::endl;
	}
}

TimerQueue::~TimerQueue(){

}

int TimerQueue::addTimer(Duration delay, std::function<void()> func){
    TimePoint expire = Clock::now() + delay;

    Timer t;
    t.id = id_count_;
    t.expireTime = expire;
    t.interval = false;
    t.callback = func;

    timers_.insert({id_count_, t});
    tasks_.insert({expire, id_count_});

    check();
    
    return id_count_++;
}

int TimerQueue::addTimer(Duration delay, Duration interval, std::function<void()> func){
    TimePoint expire = Clock::now() + delay;

    Timer t;
    t.id = id_count_;
    t.expireTime = expire;
    t.interval = interval;
    t.periodic = true;
    t.callback = func;

    timers_.insert({id_count_, t});
    tasks_.insert({expire, id_count_});
    
    check();

    return id_count_++;
}

int TimerQueue::cancelTimer(std::uint64_t id){
    timers_.erase(id);
}

void TimerQueue::readCallback(){
    read(timer_fd_, nullptr, 0);//读掉所有到期次数

    //找出到期时间 <= 当前时间的任务
    TimePoint now = Clock::now();

    std::vector<Timer> expired;

    // 第一个未到期的位置
    auto endIt = tasks_.upper_bound(now);

    //把符合条件的任务从容器中取出来
    for(auto it = tasks_.begin(); it != endIt; ++it){
        uint64_t id = it->second;

        auto tIt = timers_.find(id);
        if (tIt != timers_.end()) {
            expired.push_back(std::move(tIt->second));  // 拷/移出来
            timers_.erase(tIt);                          // 删掉完整信息
        }
    }

    //更新容器，从 multimap 里删掉已到期的
    tasks_.erase(tasks_.begin(), endIt);

    //执行回调
    for(auto& timer_ : expired){
        timer_.callback();

        //如果是周期任务，计算下一次到期时间并放回容器
        if(timer_.periodic){
            timer_.expireTime = timer_.interval;
            timers_.insert({timer_.id, timer_});
            tasks_.insert({timer_.expireTime, timer_.id});
        }
    }

    //如果还有没到期的任务，用最早的设置timerfd
    if(!tasks_.empty()){
        auto it = tasks_.begin();
        auto tIt = timers_.find(it->second);
        if(tIt != timers_.end()){
            start(tIt->second.expireTime);
        }
    }
    else stop(); //如果没有任务了，就取消timerfd的定时设置

}

void TimerQueue::start(Duration expire_time) {
    struct itimerspec its;
    std::memset(&its, 0, sizeof(its));   // 先清零，避免残留

    // 1. 转成纳秒
    auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(expire_time);
    if (ns.count() < 0) ns = std::chrono::nanoseconds::zero();  // 负数归零

    // 2. 拆分秒和纳秒
    its.it_value.tv_sec  = ns.count() / 1'000'000'000;
    its.it_value.tv_nsec = ns.count() % 1'000'000'000;

    // 3. 单次触发：interval 保持 0
    its.it_interval.tv_sec  = 0;
    its.it_interval.tv_nsec = 0;

    // 4. 启动
    timerfd_settime(timer_fd_, 0, &its, nullptr);
}

void TimerQueue::stop() {
    struct itimerspec its;
    std::memset(&its, 0, sizeof(its));   // 全部清零

    // it_value 和 it_interval 都是 0 → 停止定时器
    timerfd_settime(timer_fd_, 0, &its, nullptr);
}

void TimerQueue::check(){
    auto it = tasks_.begin();
    auto tIt = timers_.find(it->second);

    if(tIt != timers_.end()){
        start(tIt->second.expireTime);
    }
}