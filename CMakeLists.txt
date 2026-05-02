# cmake_pkg_config was added in 3.31, and it's the only way I figured out how to properly link against pjproject
# it's available from debian 13, ubuntu 25.04 and alpine 3.21
cmake_minimum_required(VERSION 3.31)
project(mumsi)


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

add_subdirectory(modules/mumlib)

find_package(Boost CONFIG COMPONENTS regex REQUIRED)

find_package(PkgConfig REQUIRED)
cmake_pkg_config(IMPORT libpjproject >=2.16 REQUIRED)
# needed for static linking workaround
# TODO can we get this through the target instead?
cmake_pkg_config(EXTRACT libpjproject >=2.16 REQUIRED)
cmake_pkg_config(IMPORT log4cpp REQUIRED)

#include_directories(${PJSIP_INCLUDE_DIRS})
#include_directories(${LOG4CPP_INCLUDE_DIRS})
include_directories(modules/mumlib/include)

#link_directories(modules/mumlib/build)

if (APPLE)
    # needed to successfully link against openssl and a bunch of other pkgconfig libraries
    link_directories(/opt/homebrew/lib)
    #include_directories(/opt/homebrew/include)
endif()

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
target_link_libraries(mumsi PkgConfig::libpjproject)
# workaround because pj is only available as static libraries, but cmake is too stupid to deal with linking this properly
# (or maybe i'm holding it wrong, who knows)
target_link_libraries(mumsi ${CMAKE_PKG_CONFIG_LIBS_PRIVATE})
#target_link_libraries(mumsi PkgConfig::LOG4CPP)
target_link_libraries(mumsi PkgConfig::log4cpp)
target_link_libraries(mumsi ${Boost_LIBRARIES})
target_link_libraries(mumsi mumlib)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        find_package(Backtrace)
        if(Backtrace_FOUND)
            include_directories(${Backtrace_INCLUDE_DIRS})
            target_link_libraries(mumsi ${Backtrace_LIBRARIES})
            add_definitions(-DUSE_DEBUG)
        endif()
endif()