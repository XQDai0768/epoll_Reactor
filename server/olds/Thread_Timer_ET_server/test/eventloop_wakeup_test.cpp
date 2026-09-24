#include "eventloop.h"

#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>

int main(){
	EventLoop loop;
	std::atomic<bool> executed{false};

	std::thread loopThread([&](){
		loop.loop();
	});

	std::this_thread::sleep_for(std::chrono::milliseconds(200));

	loop.runInLoop([&](){
		executed.store(true);
		loop.quit();
	});

	auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
	while(!executed.load() && std::chrono::steady_clock::now() < deadline){
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	if(!executed.load()){
		std::cout << "[FAIL] wakeup task was not executed in time" << std::endl;
		loop.quit();
	}

	loopThread.join();

	if(executed.load()){
		std::cout << "[PASS] cross-thread runInLoop woke up EventLoop" << std::endl;
		return 0;
	}

	return 1;
}
