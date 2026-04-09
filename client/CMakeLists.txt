cmake_minimum_required(VERSION 3.10)
project(NetLearnClient)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 寻找依赖
find_package(Boost REQUIRED COMPONENTS system)
find_package(Threads REQUIRED)

# 定义源文件
set(CLIENT_SOURCES 
    Client.cpp
)

# 生成可执行文件，名字改为 Client
add_executable(Client ${CLIENT_SOURCES})

# 链接库
target_link_libraries(Client PRIVATE 
    Boost::system
    Threads::Threads
)

# 基础编译选项
target_compile_options(Client PRIVATE -Wall -O2)