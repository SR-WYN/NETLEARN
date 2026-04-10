cmake_minimum_required(VERSION 3.10)
project(NetLearnServer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 寻找依赖
find_package(Boost REQUIRED COMPONENTS system)
find_package(Threads REQUIRED)
find_package(Protobuf REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(JSONCPP REQUIRED jsoncpp)

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

# 服务端源文件：包含主程序和共用的工具类实现
set(SERVER_SOURCES 
    Server.cpp
    Session.cpp
    MsgNode.cpp
    LogicSystem.cpp
    LogicNode.cpp
    ${PROTO_SRCS}
)

# 生成可执行文件：Server
add_executable(Server ${SERVER_SOURCES})

# 包含目录：源码目录 + protobuf 生成目录
target_include_directories(Server PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${Protobuf_INCLUDE_DIRS}
    ${JSONCPP_INCLUDE_DIRS}
)

# 链接库
target_link_libraries(Server PRIVATE 
    Boost::system
    Threads::Threads
    ${Protobuf_LIBRARIES}
    ${JSONCPP_LIBRARIES}
)

# 基础编译选项
if(JSONCPP_CFLAGS_OTHER)
    target_compile_options(Server PRIVATE ${JSONCPP_CFLAGS_OTHER})
endif()
target_compile_options(Server PRIVATE -Wall -O2)
