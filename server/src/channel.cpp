#include "channel.h"
#include "eventloop.h"

Channel::Channel(EventLoop* loop, int fd){
	fd_ = fd;
	events_ = 0;
	index_ = -1;
	loop_ = loop;
}

Channel::~Channel(){
	
}

/*
	设置读事件回调
*/
void Channel::setReadCallback(std::function<void()> func){
	readCallback_ = func;
}

/*

*/
void Channel::setWriteCallback(std::function<void()> func){
	writeCallback_ = func;
}

/*

*/
void Channel::setCloseCallback(std::function<void()> func){
	closeCallback_ = func;
}

/*

*/
void Channel::setErrorCallback(std::function<void()> func){
	errorCallback_ = func;
}

int Channel::getFd() const{
	return fd_;
}

uint32_t Channel::getEvents() const{
	return events_;
}

int Channel::getIndex() const{
	return index_;
}

void Channel::setIndex(int value){
	index_ = value;
}

/*
	加入读事件
*/
int Channel::enableReading(bool flag_ET){
	if(flag_ET) events_ |= EPOLLIN | EPOLLET;//加入边缘触发
	else events_ |= EPOLLIN;
	
	return update();
}

/*
	加入写事件
*/
int Channel::enableWriting(){
	events_ |= EPOLLOUT;
	return update();
}

/*
	禁用读事件
*/
int Channel::disableReading(){
	events_ &= ~EPOLLIN;
	return update();
}

/*
	禁用写事件
*/
int Channel::disableWriting(){
	events_ &= ~EPOLLOUT;
	return update();
}

/*
	禁用全部事件
*/
int Channel::disableAll(){
	events_ = 0;
	update();
	return 0;
}

/*
	确认是否关注读事件
*/
bool Channel::isReading() const{ 
	return (events_ & EPOLLIN) != 0;
}

/*
	确认是否关注写事件
*/
bool Channel::isWriting() const{
	return (events_ & EPOLLOUT) != 0;
}

/*

*/
void Channel::handleEvent(uint32_t revents){
	if((revents & EPOLLHUP) && !(revents & EPOLLIN) && closeCallback_) closeCallback_();

	if((revents & EPOLLERR) && errorCallback_) errorCallback_(); 

	if((revents & EPOLLIN) && readCallback_) readCallback_();

	if((revents & EPOLLOUT) && writeCallback_) writeCallback_();
}

/*

*/
int Channel::update(){
	return loop_->updateChannel(this);
}

/*

*/
void Channel::remove(){
	loop_->removeChannel(this);
}
