cmake_minimum_required(VERSION 3.10)
project(NetLearnClient)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 寻找依赖
find_package(Boost REQUIRED COMPONENTS system)
find_package(Threads REQUIRED)
find_package(Protobuf REQUIRED)
find_package(JsonCpp REQUIRED)

# Protocol Buffers 代码生成
# 支持将 .proto 文件放在单独的 proto 目录下
set(PROTO_DIR "${CMAKE_CURRENT_SOURCE_DIR}/proto")
file(GLOB PROTO_FILES "${PROTO_DIR}/*.proto")
if(PROTO_FILES)
    protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTO_FILES})
    message(STATUS "Found protobuf files: ${PROTO_FILES}")
    message(STATUS "Generated sources: ${PROTO_SRCS}")
    message(STATUS "Generated headers: ${PROTO_HDRS}")
endif()

# 定义源文件
set(CLIENT_SOURCES 
    Client.cpp
    ${PROTO_SRCS}
)

# 生成可执行文件，名字改为 Client
add_executable(Client ${CLIENT_SOURCES})

# 包含目录：源码目录 + protobuf 生成目录
target_include_directories(Client PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${Protobuf_INCLUDE_DIRS}
)

# 链接库
target_link_libraries(Client PRIVATE 
    Boost::system
    Threads::Threads
    ${Protobuf_LIBRARIES}
    JsonCpp::JsonCpp
)

# 基础编译选项
target_compile_options(Client PRIVATE -Wall -O2)
