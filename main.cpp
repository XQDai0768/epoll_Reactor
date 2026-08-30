#include <iostream>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/epoll.h>

int init_server(int port);
int setNonBlock(int fd);

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

	while(true){
		//阻塞(-1)地等待注册在epoll上的listenfd上面关注的事件(这里是读事件)的发生
		int nready = epoll_wait(epollfd, events, 1024, -1);//返回就绪事件数

		for(int i = 0; i < nready; i++){
			
			//TODO:检查events

			int fd = events[i].data.fd;
			if(fd == listenfd){//listenfd有读事件发生——有新连接
				int clientfd = accept(listenfd, nullptr, nullptr);
				if(clientfd == -1){
					perror("accept");
					continue;
				}

				//将clientfd注册进epoll，并设置为非阻塞
				setNonBlock(clientfd);
				ev.data.fd = clientfd;
				ev.events = EPOLLIN | EPOLLET;//监听读事件，设为边缘触发

				epoll_ctl(epollfd, EPOLL_CTL_ADD, clientfd, &ev);
			}
			else if(events[i].events & EPOLL_IN){
				//读取并处理数据（这里需要解决粘包问题）	
			}
		}
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
