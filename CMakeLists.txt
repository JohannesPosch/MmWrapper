cmake_minimum_required(VERSION 3.8)
project(MmWrapper)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES main.cpp
                MmFile.cpp
                MmFile.h
        condReturn.h)
add_executable(MmWrapper ${SOURCE_FILES})