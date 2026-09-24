#include "buffer.h"

Buffer::Buffer(size_t len){
	readIndex_ = 0;
	writeIndex_ = 0;
	buffer_.resize(len);
}

/*
	返回未处理的字节长度
*/
size_t Buffer::readableBytes() const{
	return writeIndex_ - readIndex_;
}

/*
	返回缓冲区还可写入的字节长度
*/
size_t Buffer::writableBytes() const{
	return buffer_.size() - writeIndex_;
}

/*
	返回可回收的字节长度
*/
size_t Buffer::prependableBytes() const{
	return readIndex_;
}

/*
	返回可读区域的首指针，让解析器去读数据
*/
const char* Buffer::peek() const{
	return buffer_.data() + readIndex_;
}

/*
	把数据追加到可写区，写指针后移
*/
int Buffer::append(const std::string data, size_t len){
	if(len == 0) return -1;

	ensureWritableBytes(len);

	memcpy(buffer_.data() + writeIndex_, data.data(), len);
	writeIndex_ += len;

	return 0;
}

/*
	确保有足够空间容纳
*/
int Buffer::ensureWritableBytes(size_t len){
	if(len > writableBytes()){
		//1.回收可回收空间
		size_t reuse_len = prependableBytes();
		if(reuse_len > 0){
			//for(size_t i = 0; i < readableBytes(); i++) buffer_[i] = buffer_[i + readIndex_];
			memmove(buffer_.data(), peek(), writeIndex_ - readIndex_);
			readIndex_ = 0;
			writeIndex_ -= reuse_len; 
		}

		//2.扩容
		if(len > writableBytes()){
			size_t need = writeIndex_ + len;

			buffer_.resize(need);
			//if(buffer_.size() < need){
				//size_t new_size = std::max(old_capacity * 2, need);
				//buffer_.resize(new_capacity);
			//}
		}
	}
	return 0;
}

/*
	从socket读数据到可写区，处理非阻塞的EAGAIN情况
*/
ReturnResult Buffer::readFd(int fd){
	
	//循环直至EAGAIN	
	while(true){
		ensureWritableBytes(65536);
		ssize_t len = read(fd, buffer_.data() + writeIndex_, writableBytes());

		if(len > 0){
			writeIndex_ += len;			
		}
		else if(len == 0){
			std::cout << fd << "关闭了连接" << std::endl;
			return ReturnResult::Closed;//提示关闭连接，清理资源
		}
		else{
			if(errno == EAGAIN || errno == EWOULDBLOCK){
				return ReturnResult::Again;
			}
			else if(errno == EINTR){
				continue;
			}
			else{
				perror("read:");
				return ReturnResult::Error;
			}
		}
	}
}

/*
	已处理完前len字节，读指针后移
*/
int Buffer::retrieve(size_t len){
	if(len > readableBytes()) return -1;
	readIndex_ += len;
	return 0;
}

/*
	清空
*/
int Buffer::retrieveAll(){
	readIndex_ = 0;
	writeIndex_ = 0;
	return 0;
}
