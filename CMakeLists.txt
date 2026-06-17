cmake_minimum_required(VERSION 3.20)
project(AutoLangC)

# Định nghĩa chuẩn C++20 như bạn yêu cầu
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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
file(GLOB_RECURSE SRC "src/*.cpp") # Quét tất cả file .cpp trong src và các thư mục con của src

# Tạo file thực thi autolang từ các file nguồn của bạn và file main của test
add_executable(autolang ${SRC} "tests/main.cpp")

# Thay vì dùng include_directories(src) toàn cục, ta dùng target_include_directories
# Điều này giúp khu biệt phạm vi include chỉ trong target autolang, tránh xung đột bừa bãi
target_include_directories(autolang PRIVATE src)

# -------------------------------------------------------------------
# 3. LIÊN KẾT LIBCURL VÀO PROJECT
# -------------------------------------------------------------------
# Định nghĩa macro biên dịch tĩnh cho Curl (bắt buộc trên Windows)
target_compile_definitions(autolang PRIVATE CURL_STATICLIB)

# Link thư viện libcurl vào project autolang của bạn
target_link_libraries(autolang PRIVATE libcurl)