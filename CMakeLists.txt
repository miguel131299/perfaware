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

# Define source files
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp)

# Define executable
add_executable(sim8086 ${SOURCES})

# Include headers in the separate include/ dir
target_include_directories(sim8086 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)