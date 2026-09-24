/*
	类功能：取代fd_buffer，动态增长，负责粘包拆包
*/
#pragma once

#include <vector>
#include <iostream>
#include <algorithm>
#include <cstring>
#include <unistd.h>//read()
#include <arpa/inet.h>//ntohl
#include <sys/types.h>//ssize_t

enum class ReturnResult{
	Again,
	Closed,
	Error
};

class Buffer{
private:
	std::vector<char> buffer_;
	size_t readIndex_;//可读区域起点
	size_t writeIndex_;//可写区域起点

public:
	Buffer(size_t len = 1024);
	~Buffer()=default ;

	size_t readableBytes() const;
	size_t writableBytes() const;
	size_t prependableBytes() const;
	const char* peek() const;

	int append(const char* data, size_t len);
	int ensureWritableBytes(size_t len);
	ReturnResult readFd(int fd);

	int retrieve(size_t len);
	int retrieveAll();

	//find()
	//retrieveUtil()
};
