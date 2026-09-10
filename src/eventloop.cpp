#include "eventloop.h"
#include "channel.h"

EventLoop::EventLoop(){
	epollfd_ = epoll_create1(EPOLL_CLOEXEC);
	if(epollfd_ == -1) perror("epoll_create:");
	events_.resize(1024);
	running_ = true;
}

EventLoop::~EventLoop(){
	if(epollfd_ != -1) close(epollfd_);	
}

void EventLoop::loop(){

	while(running_){
		int n = epoll_wait(epollfd_, events_.data(), events_.size(), -1);

		if(n == -1){
			if(errno == EINTR) continue;
			perror("epoll_wait:");
			return;
		}

		for(int i = 0; i < n; i++){
			Channel* ch = static_cast<Channel*>(events_[i].data.ptr);
			uint32_t revents = events_[i].events;
			ch->handleEvent(revents);
		}
	}
}

bool EventLoop::quit(){
	running_ = false;//wakeup机制
	return !running_;
}

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

int EventLoop::removeChannel(Channel* ch){
	if(epoll_ctl(epollfd_, EPOLL_CTL_DEL, ch->getFd(), nullptr) == -1){
		perror("epoll_ctl: DEL");
		return -1;
	}
	ch->setIndex(-1);
	return 0;
}
