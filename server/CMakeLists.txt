cmake_minimum_required(VERSION 3.10)

project(rpc_server VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)

# #查找protobuf
# find_package(protobuf CONFIG REQUIRED)

# #调用生成函数
# protobuf_generate(
#     LANGUAGE cpp
#     TARGET proto-objects
#     PROTOC_OUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated"
#     SOURCES proto/service.proto
# )

# #设置生成文件的输出目录
# target_include_directories(proto-objects PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/generated")

# #链接
# target_link_libraries(proto-objects PUBLIC protobuf::libprotobuf)

find_package(Protobuf 3.12 REQUIRED)

# protoc 不会自动创建输出目录，显式创建
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/generated")

protobuf_generate(
    LANGUAGE cpp
    APPEND_PATH
    OUT_VAR PROTO_SRCS
    PROTOC_OUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated"
    PROTOS proto/service.proto
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
)

target_include_directories(server
	PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/include

)

target_compile_options(server PRIVATE -Wall -Wextra -Wpedantic)

target_link_libraries(server PRIVATE Threads::Threads proto_lib)

#test
#add_executable(proto_test test/proto_codec_test.cpp)
#target_link_libraries(proto_test PRIVATE proto_lib)

add_executable(rpc_codec_test test/proto_codec_test.cpp src/rpc_codec.cpp src/buffer.cpp)
target_include_directories(rpc_codec_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(rpc_codec_test PRIVATE proto_lib)