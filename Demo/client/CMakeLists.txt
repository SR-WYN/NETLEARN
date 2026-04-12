cmake_minimum_required(VERSION 3.15)
project(CoroutineClientDemo LANGUAGES CXX)

# 1. 核心编译标准设置
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 让 Trae 等编辑器能够通过 compile_commands.json 获得完美的补全提示
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 2. 查找依赖
# 现代 Boost 会提供名为 Boost::xxx 的导入目标（Imported Targets）
find_package(Boost 1.83 REQUIRED COMPONENTS system)

# 3. 定义目标
add_executable(test_client TestClient.cpp)

# 4. 依赖管理（核心规范点）
# 使用 PRIVATE 关键字实现依赖隔离。
# Boost::system 会自动传播包含目录、库路径以及必要的编译标志。
target_link_libraries(test_client 
    PRIVATE 
        Boost::system
)

# 5. 编译器特性开启
# 对于 GCC，-fcoroutines 仍是开启 C++20 协程语义转换的必要开关
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(test_client PRIVATE -fcoroutines)
endif()