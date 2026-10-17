cmake_minimum_required(VERSION 3.10)

project(rpc_server VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)

find_package(Protobuf 3.12 REQUIRED)

# protoc 不会自动创建输出目录，显式创建
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/generated")

protobuf_generate(
    LANGUAGE cpp
    APPEND_PATH
    OUT_VAR PROTO_SRCS
    PROTOC_OUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated"
    PROTOS proto/service.proto proto/echo.proto proto/calculator.proto
)

add_library(proto_lib STATIC ${PROTO_SRCS})
target_include_directories(proto_lib PUBLIC
    "${CMAKE_CURRENT_BINARY_DIR}/generated"
    ${Protobuf_INCLUDE_DIRS}
)
target_link_libraries(proto_lib PUBLIC protobuf::libprotobuf)

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
	src/rpc_codec.cpp
	src/service.cpp
	src/echo_service.cpp
	src/dispatcher.cpp
	src/calculator_service.cpp
)

target_include_directories(server
	PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_options(server PRIVATE -Wall -Wextra -Wpedantic)

target_link_libraries(server PRIVATE Threads::Threads proto_lib)