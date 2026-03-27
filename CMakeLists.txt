cmake_minimum_required(VERSION 3.10)
project(mumsi)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -pthread")

add_subdirectory(modules/mumlib)

INCLUDE(FindPkgConfig)
find_package(Boost COMPONENTS system regex REQUIRED)
find_package(PkgConfig REQUIRED)

pkg_check_modules(PJSIP "libpjproject>=2.6")
pkg_check_modules(LOG4CPP "log4cpp")

add_definitions(${PJSIP_CFLAGS})
add_definitions(${LOG4CPP_CFLAGS})

include_directories(${PJSIP_INCLUDE_DIRS})
include_directories(${LOG4CPP_INCLUDE_DIRS})
include_directories(modules/mumlib/include)

link_directories(modules/mumlib/build)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release")
endif()

set(SOURCE_FILES
        PjsuaCommunicator.cpp
        PjsuaCommunicator.hpp
        MumbleCommunicator.cpp
        MumbleCommunicator.hpp
        MumbleChannelJoiner.cpp
        MumbleChannelJoiner.hpp
        Configuration.cpp
        Configuration.hpp
        IncomingConnectionValidator.cpp
        IncomingConnectionValidator.hpp
        AudioFramesMixer.hpp
        AudioFramesMixer.cpp)

add_executable(mumsi ${SOURCE_FILES} main.cpp)
target_link_libraries(mumsi pjsua2 pjsua pjmedia pj)
target_link_libraries(mumsi ${LOG4CPP_LIBRARIES})
target_link_libraries(mumsi ${Boost_LIBRARIES})
target_link_libraries(mumsi mumlib speexdsp crypto ssl protobuf opus)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        target_link_libraries(mumsi execinfo)
        add_definitions(-DUSE_DEBUG)
endif()