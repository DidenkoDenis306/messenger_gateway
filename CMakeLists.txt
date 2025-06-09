cmake_minimum_required(VERSION 3.20)
project(messenger_gateway)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)

add_library(messenger_common
        src/common/logger.cpp
        src/common/service_base.cpp
        src/services/auth_service.cpp
)

target_include_directories(messenger_common PUBLIC include)

target_link_libraries(messenger_common PUBLIC Threads::Threads)

add_executable(messenger main.cpp)

target_include_directories(messenger PRIVATE include)
target_link_libraries(messenger PRIVATE messenger_common)
