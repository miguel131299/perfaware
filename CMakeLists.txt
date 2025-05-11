cmake_minimum_required(VERSION 3.28)

project(PerfAware)

# Require C++23
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable warnings
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
  add_compile_options(-Wall -Wextra -Wpedantic)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  add_compile_options(/W4 /permissive-)
endif()

# ----------------------
# GoogleTest Setup
# ----------------------
include(FetchContent)

FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.16.0.zip
)
FetchContent_MakeAvailable(googletest)

enable_testing()

# Sources 
set(SIM8086_HEADERS 
      include/sim8086/decoder.hpp
      include/sim8086/utils.hpp
)

set(SIM8086_SOURCES 
      src/decoder/decoder.cpp
      src/utils/utils.cpp
)

# Core library
add_library(sim8086_core STATIC ${SIM8086_SOURCES})
target_include_directories(sim8086_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Main Binary
add_executable(sim8086 src/main.cpp)
target_link_libraries(sim8086 PRIVATE sim8086_core)
target_include_directories(sim8086 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Tests
add_subdirectory(tests)
