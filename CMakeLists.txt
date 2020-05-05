project(hpblog)

cmake_minimum_required(VERSION 2.8.4)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/CMake/ ${CMAKE_ROOT}/Modules/   )

add_definitions( -std=c++11 )


include_directories(
    "libs/twig/include/"
    "libs/wsrv/include/"
    "libs/xdb/include/"
    "libs/3rdparty/asio/include")

add_subdirectory(libs/twig)
add_subdirectory(libs/wsrv) 
add_subdirectory(libs/xdb)

add_subdirectory(src)

