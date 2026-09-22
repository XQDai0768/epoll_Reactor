#include <thread_pool.h>

#include <atomic>
#include <iostream>
#include <stdexcept>

int main(){
	ThreadPool pool(1);

	std::atomic<bool> throw_ran{false};
	std::atomic<int> normal_count{0};

	bool submit_ok = pool.submit([&](){
		throw_ran.store(true);
		throw std::runtime_error("expected test exception");
	});

	submit_ok = submit_ok && pool.submit([&](){
		normal_count++;
	});

	submit_ok = submit_ok && pool.submit([&](){
		normal_count++;
	});

	if(!submit_ok){
		std::cout << "[FAIL] submit failed" << std::endl;
		return 1;
	}

	pool.start();
	pool.stop();

	if(throw_ran.load() && normal_count.load() == 2){
		std::cout << "[PASS] worker survived an exception and kept processing tasks" << std::endl;
		return 0;
	}

	std::cout << "[FAIL] throw_ran=" << throw_ran.load()
			  << ", normal_count=" << normal_count.load() << std::endl;
	return 1;
}
