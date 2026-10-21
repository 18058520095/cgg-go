cmake_minimum_required(VERSION 3.16)
project(tcp_chat_room LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")

find_package(Threads REQUIRED)

add_library(lcl
    src/file/FileDescriptor.cpp
    src/net/SocketAddress.cpp
    src/net/Socket.cpp
    src/net/Epoll.cpp
    src/net/TcpClient.cpp
    src/net/ChatRoom.cpp
)
target_include_directories(lcl PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(lcl PUBLIC Threads::Threads)

foreach(app chat_server chat_client)
    add_executable(${app} apps/${app}.cpp)
    target_link_libraries(${app} PRIVATE lcl)
endforeach()

enable_testing()
add_executable(test_chat tests/test_chat.cpp)
target_link_libraries(test_chat PRIVATE lcl)
add_test(NAME chat COMMAND test_chat)
set_tests_properties(chat PROPERTIES TIMEOUT 30)
