#include <iostream>
#include<sys/socket.h>
#include<string.h>
#include<netinet/in.h>
#include<unistd.h>

int main(int argc, char* argv[]){

	//---------init-----------
	//创建socket
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

	//绑定地址和端口
	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(8888);
	addr.sin_addr.s_addr = INADDR_ANY;
	if(bind(listenfd, (sockaddr*)&addr, sizeof(addr)) == -1){
		perror("bind");
		return -1;
	}
	//----------init-----------

	//开始监听
	if(listen(listenfd, 5) == -1){
		perror("listen");
		return -1;
	}
	
	while(true){
		//阻塞等待
		int clientfd = accept(listenfd, nullptr, nullptr);
		if(clientfd == -1){
			perror("accept");
			return -1;
		}

		char buffer[1024];
		memset(&buffer, 0, sizeof(buffer));
		
		while(true){
			int n  = read(clientfd, buffer, sizeof(buffer));

			if(n == 0){
				std::cout << "客户端" << clientfd << "关闭连接" << std::endl;
				break;
			}

			if(n < 0){
				perror("read");
				break;
			}

			std::cout << "收到：";
			std::cout.write(buffer, sizeof(buffer));
			write(clientfd, buffer, n);//原样发回去
		}
		close(clientfd);
	}
	close(listenfd);
	return 0;
}
