cmake_minimum_required(VERSION 3.20)
project(AutoLangC_Examples CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include AutoLangC root directory
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../src)

# Search all AutoLangC source files
file(GLOB_RECURSE AUTOLANGC_SRC "${CMAKE_CURRENT_SOURCE_DIR}/../src/*.cpp")

# Examples project setup

