#include "thread_pool.h"
#include <iostream>
#include <atomic>
#include <unistd.h>

int main(int argc, char* argv[]){
	ThreadPool th(20);
	std::atomic<int> num = 0;

	for(int i = 0; i < 20; i++){
		th.submit([&](){
			num++;
			sleep(10);
		});
	}

	th.start();
	th.stop();

	std::cout << "num=" << num << std::endl;
}
