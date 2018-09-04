cmake_minimum_required(VERSION 3.0)
project(example-server)

if (MSVC)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -std=c++11")
endif()

if (WIN32)
    add_definitions(-DWEBRTC_WIN)
endif()

set(BOOST_ROOT "D:/Program Files/boost_1_68_0")
set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_STATIC_RUNTIME ON)
find_package(Boost REQUIRED system date_time regex)
include_directories(${Boost_INCLUDE_DIRS})


include_directories("${CMAKE_CURRENT_SOURCE_DIR}/deps/target/include")
if (MSVC)
    link_directories("${CMAKE_CURRENT_SOURCE_DIR}/deps/target/lib/Debug")
    link_directories("${CMAKE_CURRENT_SOURCE_DIR}/deps/target/lib/Release")
else()
    link_directories("${CMAKE_CURRENT_SOURCE_DIR}/deps/target/lib")
endif()

include_directories("src/")
include_directories("lib/")
include_directories(${CMAKE_CURRENT_BINARY_DIR})

file(GLOB PROJECT_CONFIGS CMakeLists.txt
                         .gitattributes
                         .gitignore
                         .gitmodules)

add_executable(server src/main.cpp src/observers.h)
target_link_libraries(server ${Boost_LIBRARIES} webrtc Secur32 Msdmo Winmm Dmoguids wmcodecdspuuid)