cmake_minimum_required(VERSION 3.10)

project(tcp_server VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)

add_executable(server 
	src/main.cpp
	src/buffer.cpp
	src/channel.cpp
	src/eventloop.cpp
	src/acceptor.cpp
	src/tcp_connection.cpp
	src/tcp_server.cpp
	src/thread_pool.cpp
	src/timer_queue.cpp
)

target_include_directories(server
	PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/include

)

target_compile_options(server PRIVATE -Wall -Wextra -Wpedantic)

target_link_libraries(server PRIVATE Threads::Threads)
