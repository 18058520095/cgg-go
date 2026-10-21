cmake_minimum_required(VERSION 3.16)
project(cloudisk_drogon CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Drogon CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)

add_executable(cloudisk_drogon
    main.cc
    ApiCtrl.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/../Hash.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/../Token.cc
)

target_include_directories(cloudisk_drogon PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(cloudisk_drogon PRIVATE Drogon::Drogon OpenSSL::SSL OpenSSL::Crypto)
