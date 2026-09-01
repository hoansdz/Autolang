cmake_minimum_required(VERSION 3.20)
project(AutoLangC LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MINGW)
  add_compile_options(-Wa,-mbig-obj)
endif()

# -------------------------------------------------------------------
# 1. DEPENDENCIES (FetchContent)
# -------------------------------------------------------------------
include(FetchContent)

# Sau lần configure đầu tiên, dependency đã tải xong trong _deps/.
# Bật ON để bỏ qua kiểm tra git remote mỗi lần re-configure.
option(FETCHCONTENT_UPDATES_DISCONNECTED
       "Bỏ qua kiểm tra cập nhật dependency (bật sau lần tải đầu)" ON)

# --- libcurl (static) ---
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(BUILD_CURL_EXE    OFF CACHE BOOL "" FORCE)
set(BUILD_TESTING     OFF CACHE BOOL "" FORCE)
set(ENABLE_CURL_MANUAL OFF CACHE BOOL "" FORCE)
set(BUILD_LIBCURL_DOCS OFF CACHE BOOL "" FORCE)
set(CURL_DISABLE_LDAP  ON CACHE BOOL "" FORCE)
set(CURL_DISABLE_LDAPS ON CACHE BOOL "" FORCE)
set(CMAKE_WARN_DEPRECATED OFF CACHE BOOL "" FORCE)

FetchContent_Declare(curl
  GIT_REPOSITORY https://github.com/curl/curl.git
  GIT_TAG        curl-8_8_0
  GIT_SHALLOW    ON
)

# --- rapidfuzz ---
FetchContent_Declare(rapidfuzz
  GIT_REPOSITORY https://github.com/rapidfuzz/rapidfuzz-cpp.git
  GIT_TAG        v3.3.2
  GIT_SHALLOW    ON
)

FetchContent_MakeAvailable(curl rapidfuzz)

# -------------------------------------------------------------------
# 2. AUTOLANG STATIC LIBRARY
# -------------------------------------------------------------------
file(GLOB_RECURSE SRC CONFIGURE_DEPENDS "src/*.cpp")
list(FILTER SRC EXCLUDE REGEX ".*AVM_run_(computed_goto|switch)\\.cpp$")

add_library(autolang_lib STATIC ${SRC})

target_include_directories(autolang_lib PUBLIC src)

target_compile_definitions(autolang_lib PUBLIC
  CURL_STATICLIB
  NOMINMAX
  _CRT_SECURE_NO_WARNINGS
  AUTOLANG_LIMIT_OPCODE
)

target_link_libraries(autolang_lib PUBLIC libcurl rapidfuzz)

# -------------------------------------------------------------------
# 3. AUTOLANG EXECUTABLE + CONVENIENCE TARGETS
# -------------------------------------------------------------------
add_executable(autolang "tests/main.cpp")
target_compile_definitions(autolang PRIVATE NOMINMAX _CRT_SECURE_NO_WARNINGS)
target_link_libraries(autolang PRIVATE autolang_lib)

# "cmake --build build --target run" để build và chạy ngay
add_custom_target(run
  COMMAND ${CMAKE_COMMAND} -E chdir ${CMAKE_SOURCE_DIR} $<TARGET_FILE:autolang>
  DEPENDS autolang
  USES_TERMINAL
)