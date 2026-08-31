#include <iostream>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <unordered_map>

int init_server(int port);
int setNonBlock(int fd);
int handleRead(int fd, std::unordered_map<int, fd_buffer>& buffers);

struct fd_buffer{
	size_t length;
	char data[1024];
};

int main(int argc, char* argv[]){
	int listenfd = init_server(atoi(argv[1]));

	//创建epoll句柄
	int epollfd = epoll_create(1);

	//事件初始化
	epoll_event ev;
	ev.data.fd = listenfd;//绑定事件监听的sokcet，这里是监听listenfd
	ev.events = EPOLLIN;//绑定事件监听的类型，这里是监听读事件

	//注册，把需要监视的socket和事件加入epoll中
	epoll_ctl(epollfd, EPOLL_CTL_ADD, listenfd, &ev);

	epoll_event events[1024];
	std::unordered_map<int, fd_buffer> Buffers;

	while(true){
		//阻塞(-1)地等待注册在epoll上的listenfd上面关注的事件(这里是读事件)的发生
		int nready = epoll_wait(epollfd, events, 1024, -1);//返回就绪事件数

		for(int i = 0; i < nready; i++){
			
			//TODO:检查events

			int fd = events[i].data.fd;
			if(fd == listenfd){//listenfd有读事件发生——有新连接
				int clientfd = accept(listenfd, nullptr, nullptr);
				if(clientfd == -1 && errno != EAGAIN){
					perror("accept");
					continue;
				}
				//为新clientfd注册read缓冲区
				struct fd_buffer buffer;
				Buffers.emplace(clientfd, buffer);//C++11, 效率最高，避免拷贝
				//Buffers.insert({clientfd, buffer})

				//将clientfd注册进epoll，并设置为非阻塞
				setNonBlock(clientfd);
				ev.data.fd = clientfd;
				ev.events = EPOLLIN | EPOLLET;//监听读事件，设为边缘触发

				epoll_ctl(epollfd, EPOLL_CTL_ADD, clientfd, &ev);
			}
			else if(events[i].events & EPOLLIN){//条件判断：如果发生的事件是读事件
				//读取并处理数据（这里需要解决粘包问题）
				handleRead(fd, Buffers);
			}
		}
	}

	return 0;
}

int handleRead(int fd, std::unordered_map<int, fd_buffer>& buffers){
	//设计一个最简单的协议（4字节长度 + 数据）来解决粘包问题
	auto it = buffers.find(fd);
	if(it == buffers.end()){
		return -1;//错误：未找到此fd的缓冲区 TODO：为这个fd分配缓冲区后再return -1;
	}

	struct fd_buffer& buf = it->second;
	size_t remaining = sizeof(buf.data) - buf.length - 1;
	if(remaining <= 0) return -1;//空间不足

	ssize_t len = read(fd, buf.data + buf.length, remaining);
	if(len < 0){
		if(errno == EAGAIN || errno == EWOULDBLOCK) return 0;//非阻塞模式下暂无数据

		perror("read");
		return -1;
	}else if(len == 0){
		std::cout << fd << "关闭了连接" << std::endl;
		return 0;
	}

	buf.length += len;
	buf.data[length] = '\0';

	int total_len = 0;
	//读取缓冲区，看看现在能不能读出有效数据
	while(buf.length > 4){
		//解析消息长度
		int msg_len = *(int *)(buf.data);//TODO:处理字节序问题

		//检查长度是否合法
		if(msg_len < 0 | msg_len > 1024){
			//清空缓冲区，避免死循环
			buf.length = 0;
			return -1;
		}

		//判断能否组成完整消息
		int total_need = 4 + msg_len;//4字节头 + 数据体
		if(buf.length < total_need) break;

		//读取完整消息
		std::cout << "读取完整消息，长度:" << msg_len << std::endl;
		//处理消息，写回或者回复

		//移除已处理消息
		memmove(buf.data, buf.data + total_need, buf.length - total_need);
		buf.length -= total_need;
	}
	return 0;
}

int init_server(int port){
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if(listenfd == -1){
        perror("socket");
        return -1;
    }

    int opt = 1;
    if(setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1){
        perror("setsockopt");
        close(listenfd);
        return -1;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = port;
    addr.sin_addr.s_addr = INADDR_ANY;

    if(bind(listenfd, (sockaddr*)&addr, sizeof(addr)) == -1){
        perror("bind");
        close(listenfd);
        return -1;
    }

    //将listenfd设为非阻塞
    setNonBlock(listenfd);

    return listenfd;
}

int setNonBlock(int fd){
	//获取当前标志位
	int flags = fcntl(fd, F_GETFL, 0);
	if(flags == -1) return -1;

	//添加O_NONBLOCK标志
	flags |= O_NONBLOCK;
	//flags = flags | O_NONBLOCK;

	//将新标志位写回
	return fcntl(fd, F_SETFL, flags);
}
