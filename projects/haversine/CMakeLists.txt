# Haversine Project - Haversine distance calculator and data generator

set(HAVERSINE_SOURCES
      ${CMAKE_CURRENT_SOURCE_DIR}/src/haversine.cpp
)

set(HAVERSINE_HEADERS
      ${CMAKE_CURRENT_SOURCE_DIR}/include/haversine/haversine.hpp
)

# Core library
add_library(haversine STATIC ${HAVERSINE_SOURCES})
target_include_directories(haversine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Main executable - haversine data generator
add_executable(haversine_gen ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)
target_link_libraries(haversine_gen PRIVATE haversine)
target_include_directories(haversine_gen PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
