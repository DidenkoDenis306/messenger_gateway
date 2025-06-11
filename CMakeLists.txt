cmake_minimum_required(VERSION 3.20)
project(messenger_gateway)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

cmake_policy(SET CMP0135 NEW)

find_package(Threads REQUIRED)

# Include FetchContent for downloading dependencies
include(FetchContent)

# Add httplib for HTTP server functionality
FetchContent_Declare(
        httplib
        GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
        GIT_TAG v0.14.1
)
FetchContent_MakeAvailable(httplib)

# Add nlohmann/json for JSON processing
FetchContent_Declare(
        json
        URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(json)

# Create common library
add_library(messenger_common
        src/common/logger.cpp
        src/common/service_base.cpp
        src/common/http_service.cpp
        src/services/auth_service.cpp
        src/services/user_service.cpp
        src/services/message_service.cpp
        src/services/websocket_service.cpp
)

target_include_directories(messenger_common PUBLIC include)
target_link_libraries(messenger_common PUBLIC
        Threads::Threads
        httplib::httplib
        nlohmann_json::nlohmann_json
)

# Main executable
add_executable(messenger main.cpp)
target_include_directories(messenger PRIVATE include)
target_link_libraries(messenger PRIVATE messenger_common)