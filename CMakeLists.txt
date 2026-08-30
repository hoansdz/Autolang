cmake_minimum_required(VERSION 3.20)
project(AutoLangC)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MINGW)
  add_compile_options(-Wa,-mbig-obj)
endif()

# -------------------------------------------------------------------
# 1. AUTO DOWNLOAD AND BUILD DEPENDENCIES
# -------------------------------------------------------------------
include(FetchContent)

# Sau lần build đầu tiên, bật ON để tránh kiểm tra repo mỗi lần configure
option(FETCHCONTENT_UPDATES_DISCONNECTED "Không kiểm tra cập nhật dependency" OFF)

# --- Curl Configuration ---
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries")
set(BUILD_CURL_EXE OFF CACHE BOOL "Build curl executable")
set(BUILD_TESTING OFF CACHE BOOL "Build curl tests")
set(CURL_DISABLE_LDAP ON CACHE BOOL "Disable LDAP")
set(CURL_DISABLE_LDAPS ON CACHE BOOL "Disable LDAPS")

FetchContent_Declare(
  curl
  GIT_REPOSITORY https://github.com/curl/curl.git
  GIT_TAG        curl-8_8_0
  GIT_SHALLOW    ON
)

# --- Rapidfuzz Configuration ---
FetchContent_Declare(
  rapidfuzz
  GIT_REPOSITORY https://github.com/rapidfuzz/rapidfuzz-cpp.git
  GIT_TAG        v3.3.2
  GIT_SHALLOW    ON
)

FetchContent_MakeAvailable(curl rapidfuzz)

# -------------------------------------------------------------------
# 2. AUTOLANGC SOURCE CONFIGURATION
# -------------------------------------------------------------------
file(GLOB_RECURSE SRC CONFIGURE_DEPENDS "src/*.cpp")
list(FILTER SRC EXCLUDE REGEX ".*AVM_run_(computed_goto|switch)\\.cpp$")

# Create a static library for embedding AutoLangC in other projects
add_library(autolang_lib STATIC ${SRC})
target_include_directories(autolang_lib PUBLIC src)
target_compile_definitions(autolang_lib PUBLIC CURL_STATICLIB NOMINMAX _CRT_SECURE_NO_WARNINGS AUTOLANG_LIMIT_OPCODE)

# Link libcurl and rapidfuzz to autolang_lib
target_link_libraries(autolang_lib PUBLIC libcurl rapidfuzz)

# Create the autolang executable from source and test main
add_executable(autolang "tests/main.cpp")
target_compile_definitions(autolang PRIVATE NOMINMAX _CRT_SECURE_NO_WARNINGS)
target_link_libraries(autolang PRIVATE autolang_lib)

# Add a convenience target to run quickly using "cmake --build build --target run"
add_custom_target(run
  COMMAND ${CMAKE_COMMAND} -E chdir ${CMAKE_SOURCE_DIR} $<TARGET_FILE:autolang>
  DEPENDS autolang
  USES_TERMINAL
)