cmake_minimum_required(VERSION 3.16)
project(metriqos VERSION 0.1.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(CURL REQUIRED)
include(FetchContent)
FetchContent_Declare(json
  URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz)
FetchContent_MakeAvailable(json)

add_library(sqlite3 STATIC third_party/sqlite3/sqlite3.c)
target_include_directories(sqlite3 PUBLIC third_party/sqlite3)

file(GLOB_RECURSE SDK_SOURCES src/*.cpp)
add_library(metriqos STATIC ${SDK_SOURCES})
target_include_directories(metriqos PUBLIC include PRIVATE src)
target_link_libraries(metriqos PRIVATE CURL::libcurl nlohmann_json::nlohmann_json sqlite3)

if(WIN32)
  target_link_libraries(metriqos PRIVATE ws2_32)
endif()

option(METRIQOS_BUILD_TESTS "Build tests" OFF)
if(METRIQOS_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()

option(METRIQOS_BUILD_EXAMPLES "Build examples" OFF)
if(METRIQOS_BUILD_EXAMPLES)
  add_subdirectory(examples/raylib)
endif()
