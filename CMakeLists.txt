cmake_minimum_required(VERSION 3.20)
project(AutoLangC)

# Định nghĩa chuẩn C++17
set(CMAKE_CXX_STANDARD 17)
if(MINGW)
    add_compile_options(-Wa,-mbig-obj)
endif()

# -------------------------------------------------------------------
# 1. TỰ ĐỘNG TẢI VÀ BIÊN DỊCH LIBCURL (STATIC LINK)
# -------------------------------------------------------------------
include(FetchContent)

FetchContent_Declare(
    curl
    GIT_REPOSITORY https://github.com/curl/curl.git
    GIT_TAG        curl-8_8_0
)

# Tối ưu hóa: Tắt các tính năng thừa của curl để build nhanh hơn
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries" FORCE)
set(BUILD_CURL_EXE OFF CACHE BOOL "Build curl executable" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "Build curl tests" FORCE)
set(CURL_DISABLE_LDAP ON CACHE BOOL "Disable LDAP" FORCE)
set(CURL_DISABLE_LDAPS ON CACHE BOOL "Disable LDAPS" FORCE)

FetchContent_MakeAvailable(curl)

# -------------------------------------------------------------------
# 2. CẤU HÌNH SOURCE CODE CHO AUTOLANGC
# -------------------------------------------------------------------
# Lưu ý: file(GLOB) không tự nhận diện khi bạn tạo file .cpp mới (phải clear cache CMake).
# Cách tốt nhất và chuẩn khuyến nghị của CMake là liệt kê rõ ràng các file nguồn:
file(GLOB_RECURSE SRC "src/*.cpp") # Quét tất cả file .cpp trong src
list(FILTER SRC EXCLUDE REGEX ".*AVM_run_computed_goto\\.cpp$")

# Tạo library cho việc nhúng AutoLangC vào dự án khác (C++ Embedding)
add_library(autolang_lib STATIC ${SRC})
target_include_directories(autolang_lib PUBLIC src)
target_compile_definitions(autolang_lib PUBLIC CURL_STATICLIB)
target_link_libraries(autolang_lib PUBLIC libcurl)

# Tạo file thực thi autolang từ các file nguồn của bạn và file main của test
add_executable(autolang "tests/main.cpp")
target_link_libraries(autolang PRIVATE autolang_lib)