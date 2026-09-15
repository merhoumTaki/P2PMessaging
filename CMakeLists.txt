cmake_minimum_required(VERSION 3.10)

project(P2PMessagingApp C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_executable(P2PMessagingApp main.c main.h)

if(WIN32)

    target_link_libraries(P2PNetworkApp PRIVATE ws2_32)
else()

    find_package(Threads REQUIRED)
    target_link_libraries(P2PMessagingApp PRIVATE Threads::Threads)
endif()
