cmake_minimum_required(VERSION 3.20)
project(AutoLangC LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MINGW)
  add_compile_options(-Wa,-mbig-obj)
endif()

if(EMSCRIPTEN)
  add_compile_options(-fexceptions)
endif()

# -------------------------------------------------------------------
# 1. DEPENDENCIES (FetchContent)
# -------------------------------------------------------------------
include(FetchContent)

# After initial configure, dependencies are downloaded in _deps/.
# Turn ON to skip git remote checks on subsequent re-configures.
option(FETCHCONTENT_UPDATES_DISCONNECTED
       "Skip dependency update checks (enable after initial download)" ON)

# --- libcurl (static) ---
if(NOT EMSCRIPTEN)
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
  FetchContent_MakeAvailable(curl)
endif()

# --- rapidfuzz ---
FetchContent_Declare(rapidfuzz
  GIT_REPOSITORY https://github.com/rapidfuzz/rapidfuzz-cpp.git
  GIT_TAG        v3.3.2
  GIT_SHALLOW    ON
)

FetchContent_MakeAvailable(rapidfuzz)

# -------------------------------------------------------------------
# 2. AUTOLANG STATIC LIBRARY
# -------------------------------------------------------------------
file(GLOB_RECURSE SRC CONFIGURE_DEPENDS "src/*.cpp")
list(FILTER SRC EXCLUDE REGEX ".*AVM_run_(computed_goto|switch)\\.cpp$")

add_library(autolang_lib STATIC ${SRC})

target_include_directories(autolang_lib PUBLIC src)

target_compile_definitions(autolang_lib PUBLIC
  NOMINMAX
  _CRT_SECURE_NO_WARNINGS
  AUTOLANG_LIMIT_OPCODE
)

if(NOT EMSCRIPTEN)
  target_compile_definitions(autolang_lib PUBLIC CURL_STATICLIB)
  target_link_libraries(autolang_lib PUBLIC libcurl)
endif()

target_link_libraries(autolang_lib PUBLIC rapidfuzz)

# -------------------------------------------------------------------
# 3. TARGETS
# -------------------------------------------------------------------
if(EMSCRIPTEN)
  add_executable(autolang_wasm "tests/wasm/main.cpp")
  target_link_libraries(autolang_wasm PRIVATE autolang_lib)

  set_target_properties(autolang_wasm PROPERTIES
    OUTPUT_NAME "autolang"
    SUFFIX ".js"
    RUNTIME_OUTPUT_DIRECTORY "D:/code/AutoLangNpm/bin"
  )

  target_link_options(autolang_wasm PRIVATE
    "-O2"
    "-gsource-map"
    "-fexceptions"
    "--bind"
    "-sASSERTIONS=1"
    "-sALLOW_MEMORY_GROWTH=1"
    "-sINITIAL_MEMORY=10MB"
    "-sASYNCIFY"
    "-sFETCH=1"
    "-sMODULARIZE=1"
    "-sEXPORT_NAME=AutoLangModule"
    "-sENVIRONMENT=web,node"
  )
else()
  add_executable(autolang "tests/main.cpp")
  target_compile_definitions(autolang PRIVATE NOMINMAX _CRT_SECURE_NO_WARNINGS)
  target_link_libraries(autolang PRIVATE autolang_lib)

  # "cmake --build build --target run" to build and run immediately
  add_custom_target(run
    COMMAND ${CMAKE_COMMAND} -E chdir ${CMAKE_SOURCE_DIR} $<TARGET_FILE:autolang>
    DEPENDS autolang
    USES_TERMINAL
  )
endif()