cmake_minimum_required(VERSION 3.15)
project(TestClient LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Boost 1.83 REQUIRED COMPONENTS system)
find_package(PkgConfig REQUIRED)
pkg_check_modules(JSONCPP REQUIRED jsoncpp)

add_executable(TestClient
    TestClient.cpp
)

target_link_libraries(TestClient
    PRIVATE
        Boost::system
        ${JSONCPP_LIBRARIES}
)

target_include_directories(TestClient
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${JSONCPP_INCLUDE_DIRS}
)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(TestClient PRIVATE -fcoroutines)
endif()

if(JSONCPP_CFLAGS_OTHER)
    target_compile_options(TestClient PRIVATE ${JSONCPP_CFLAGS_OTHER})
endif()