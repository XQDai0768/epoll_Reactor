cmake_minimum_required(VERSION 3.10)

project(epoll_Reactor VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(server 
	src/main.cpp
	src/buffer.cpp
	src/channel.cpp
	src/eventloop.cpp
	src/acceptor.cpp
	src/tcp_connection.cpp
)

target_include_directories(server
	PRIVATE
	${CAMKE_CURRENT_SOURCE_DIR}/include

)

target_compile_options(server PRIVATE -Wall -Wextra -Wpedantic)
