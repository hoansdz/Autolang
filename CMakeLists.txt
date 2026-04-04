cmake_minimum_required(VERSION 3.20)

project(AutoLangC)

set(CMAKE_CXX_STANDARD 20)

# include folder
include_directories(src)

# tự động lấy source
file(GLOB SRC
    "src/*.cpp"
    "tests/main.cpp"
)

add_executable(autolang ${SRC})