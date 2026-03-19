cmake_minimum_required(VERSION 3.16)
project(metriqos VERSION 0.1.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

FetchContent_Declare(json
  URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz)

set(BUILD_CURL_EXE OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(HTTP_ONLY ON CACHE BOOL "" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
if(WIN32)
  set(CURL_USE_SCHANNEL ON CACHE BOOL "" FORCE)
  set(CURL_USE_OPENSSL OFF CACHE BOOL "" FORCE)
elseif(APPLE)
  set(CURL_USE_SECTRANSP ON CACHE BOOL "" FORCE)
  set(CURL_USE_OPENSSL OFF CACHE BOOL "" FORCE)
else()
  set(CURL_USE_OPENSSL ON CACHE BOOL "" FORCE)
endif()
FetchContent_Declare(curl
  URL https://github.com/curl/curl/releases/download/curl-8_7_1/curl-8.7.1.tar.xz)

FetchContent_MakeAvailable(json curl)

add_library(sqlite3 STATIC third_party/sqlite3/sqlite3.c)
target_include_directories(sqlite3 PUBLIC third_party/sqlite3)

file(GLOB_RECURSE SDK_SOURCES src/*.cpp)
add_library(metriqos STATIC ${SDK_SOURCES})
target_include_directories(metriqos PUBLIC include PRIVATE src)
target_link_libraries(metriqos PRIVATE libcurl_static nlohmann_json::nlohmann_json sqlite3)

if(WIN32)
  target_link_libraries(metriqos PRIVATE ws2_32)
endif()

# Tests and examples only when building standalone (not as subdirectory)
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  option(METRIQOS_BUILD_TESTS "Build tests" OFF)
  if(METRIQOS_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
  endif()

  option(METRIQOS_BUILD_EXAMPLES "Build examples" OFF)
  if(METRIQOS_BUILD_EXAMPLES)
    add_subdirectory(examples/raylib)
  endif()
endif()
