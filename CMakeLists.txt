cmake_minimum_required(VERSION 3.25)
project(EchoServ)

set(CMAKE_CXX_STANDARD 14)

add_executable(EchoServ
        server.cpp)

add_executable(EchoCli
        client.cpp)
