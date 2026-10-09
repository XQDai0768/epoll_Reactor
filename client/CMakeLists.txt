cmake_minimum_required(VERSION 3.10)

project(rpc_client VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Protobuf 3.12 REQUIRED)

# protoc 不会自动创建输出目录，显式创建
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/generated")

protobuf_generate(
    LANGUAGE cpp
    APPEND_PATH
    OUT_VAR PROTO_SRCS
    PROTOC_OUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated"
    PROTOS ../server/proto/service.proto ../server/proto/echo.proto
)

add_library(proto_lib STATIC ${PROTO_SRCS})
target_include_directories(proto_lib PUBLIC
    "${CMAKE_CURRENT_BINARY_DIR}/generated"
    ${Protobuf_INCLUDE_DIRS}
)
target_link_libraries(proto_lib PUBLIC protobuf::libprotobuf)

add_executable(client
    main.cpp
    ../server/src/buffer.cpp
    ../server/src/rpc_codec.cpp
)

target_include_directories(client PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../server/include
)

target_link_libraries(client PRIVATE proto_lib)
