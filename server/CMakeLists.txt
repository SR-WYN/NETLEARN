cmake_minimum_required(VERSION 3.10)
project(NetLearnServer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 寻找依赖
find_package(Boost REQUIRED COMPONENTS system)
find_package(Threads REQUIRED)

# 服务端源文件：包含主程序和共用的工具类实现
set(SERVER_SOURCES 
    Server.cpp
    Session.cpp
)

# 生成可执行文件：Server
add_executable(Server ${SERVER_SOURCES})

# 链接库
target_link_libraries(Server PRIVATE 
    Boost::system
    Threads::Threads
)

# 基础编译选项
target_compile_options(Server PRIVATE -Wall -O2)