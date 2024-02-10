cmake_minimum_required(VERSION 3.13)
project(http-server-starter-cpp)
set(CMAKE_CXX_STANDARD 20) # Enable the C++20 standard

add_executable(server src/server.cpp
        src/util/util.h
        src/util/util.cpp
        src/http/http_request.cpp
        src/http/http_request.h
)

find_package(Threads REQUIRED)
target_link_libraries(server PRIVATE Threads::Threads)
