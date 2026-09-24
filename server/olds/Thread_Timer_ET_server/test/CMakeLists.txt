cmake_minimum_required(VERSION 3.10)

project(test_project VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)

add_executable(thread_pool_exception_test
	thread_pool_exception_test.cpp
	../src/thread_pool.cpp
)

target_include_directories(thread_pool_exception_test
	PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_compile_options(thread_pool_exception_test PRIVATE -Wall -Wextra -Wpedantic)

target_link_libraries(thread_pool_exception_test PRIVATE Threads::Threads)
