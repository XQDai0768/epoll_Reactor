#include "eventloop.h"
#include "channel.h"

EventLoop::EventLoop(){
	epollfd_ = epoll_create1(EPOLL_CLOEXEC);
	if(epollfd_ == -1) perror("epoll_create:");

	events_.resize(1024);
	running_ = true;

	eventfd_ = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
	if(eventfd_ == -1) perror("eventfd:");

	//注册eventfd_
	struct epoll_event ev = {};
	ev.events = EPOLLIN;
	ev.data.fd = eventfd_;

	if(epoll_ctl(epollfd_, EPOLL_CTL_ADD, eventfd_, &ev) == -1)
		perror("eventfd epoll_ctl:");

	timer_ = std::make_unique<TimerQueue>(this);
}

EventLoop::~EventLoop(){
	if(epollfd_ != -1) close(epollfd_);	

	if(eventfd_ != - 1) close(eventfd_);
}

/*
	事件循环
*/
void EventLoop::loop(){

	std::vector<std::function<void()>> local_;

	while(running_){
		
		//先清理一次 [锁内交换，锁外执行]！！！
		{
			std::lock_guard<std::mutex> lock(mtx_);
			local_.swap(pendingTasks_);
			pendingTasks_.clear();
		}
		for(auto task : local_){
			task();
		}
		local_.clear();

		int n = epoll_wait(epollfd_, events_.data(), events_.size(), -1);

		if(n == -1){
			if(errno == EINTR) continue;
			perror("epoll_wait:");
			return;
		}

		for(int i = 0; i < n; i++){
			
			if(events_[i].data.fd == eventfd_){
				uint64_t val;
				ssize_t n = read(eventfd_, &val, sizeof(val));
				if(n == -1){
					perror("eventfd read:");
				}
				continue;
			}

			Channel* ch = static_cast<Channel*>(events_[i].data.ptr);
			uint32_t revents = events_[i].events;
			ch->handleEvent(revents);
		}

		//关闭失效连接
		{
			std::lock_guard<std::mutex> lock(mtx_);
			local_.swap(pendingTasks_);
			pendingTasks_.clear();
		}
		for(auto task : local_){
			task();
		}
		local_.clear();
	}
}

/*
	终止循环
*/
bool EventLoop::quit(){
	running_ = false;

	uint64_t one = 1;
	ssize_t n = write(eventfd_, &one, sizeof(one));
	if(n == -1){
		perror("eventfd write:");
		return false;
	}

	return !running_;
}

void EventLoop::runInLoop(std::function<void()> task){
	{
		std::lock_guard<std::mutex> lock(mtx_);
		pendingTasks_.emplace_back(task);
	}

	//让阻塞在 epoll_wait/poll/read 上的线程醒过来
	uint64_t one = 1;
	ssize_t n = write(eventfd_, &one, sizeof(one));
	if(n == -1){
		perror("eventfd write:");
	}
}

/*
	注册或更新Channel
*/
int EventLoop::updateChannel(Channel* ch){
	int fd = ch->getFd();
	uint32_t events = ch->getEvents();
	int index = ch->getIndex();

	epoll_event ev = {};//零初始化
	ev.events = events;
	ev.data.ptr = ch;

	if(index == -1){//还没注册
		if(epoll_ctl(epollfd_, EPOLL_CTL_ADD, fd, &ev) == -1){
			perror("epoll_ctl: ADD");
			return -1;
		}
		ch->setIndex(1);
	}
	else{
		if(epoll_ctl(epollfd_, EPOLL_CTL_MOD, fd, &ev) == -1){
			perror("epoll_ctl: MOD");
			return -1;
		}
	}

	return 0;
}

/*
	注销Channel
*/
int EventLoop::removeChannel(Channel* ch){
	if(epoll_ctl(epollfd_, EPOLL_CTL_DEL, ch->getFd(), nullptr) == -1){
		perror("epoll_ctl: DEL");
		return -1;
	}
	ch->setIndex(-1);
	return 0;
}

uint64_t EventLoop::addTimer(Duration delay, std::function<void()> func){
	return timer_->addTimer(delay, func);
}

uint64_t EventLoop::addTimer(Duration delay, Duration interval, std::function<void()> func){
	return timer_->addTimer(delay, interval, func);
}

void EventLoop::cancelTimer(std::uint64_t id){
	timer_->cancelTimer(id);
}