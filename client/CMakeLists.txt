cmake_minimum_required(VERSION 3.5.0)
project(chat-project VERSION 0.1.0 LANGUAGES C CXX)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(server server.cpp)
add_executable(client client.cpp)
