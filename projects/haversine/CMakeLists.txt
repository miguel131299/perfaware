# Haversine Project - Haversine distance calculator and data generator

# Option to enable/disable profiling
option(ENABLE_PROFILING "Enable profiling in haversine processor" ON)

set(HAVERSINE_SOURCES
      ${CMAKE_CURRENT_SOURCE_DIR}/src/haversine.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/src/parser.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/src/processor.cpp
)

set(HAVERSINE_HEADERS
      ${CMAKE_CURRENT_SOURCE_DIR}/include/haversine/haversine.hpp
      ${CMAKE_CURRENT_SOURCE_DIR}/include/haversine/parser.hpp
      ${CMAKE_CURRENT_SOURCE_DIR}/include/haversine/processor.hpp
)

# Core library
add_library(haversine STATIC ${HAVERSINE_SOURCES})
target_include_directories(haversine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Main executable - haversine data generator
add_executable(haversine_gen ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)
target_link_libraries(haversine_gen PRIVATE haversine)
target_include_directories(haversine_gen PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Haversine processor executable
add_executable(haversine_processor ${CMAKE_CURRENT_SOURCE_DIR}/src/processor_main.cpp)
target_link_libraries(haversine_processor PRIVATE haversine)
target_include_directories(haversine_processor PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Apply profiling option
if(NOT ENABLE_PROFILING)
  target_compile_definitions(haversine_processor PRIVATE ENABLE_PROFILING=0)
endif()

# Repetition tester executable
add_executable(repetition_tester ${CMAKE_CURRENT_SOURCE_DIR}/src/tools/repetition_test_main.cpp)
target_include_directories(repetition_tester PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
# Page fault test executable
add_executable(page_fault_tester ${CMAKE_CURRENT_SOURCE_DIR}/src/page_faults/page_fault_test_main.cpp)
target_include_directories(page_fault_tester PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Pointer analysis executable
add_executable(pointer_analyzer ${CMAKE_CURRENT_SOURCE_DIR}/src/page_faults/pointer_analysis_main.cpp)
target_include_directories(pointer_analyzer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Page fault analysis executable
add_executable(page_fault_analysis ${CMAKE_CURRENT_SOURCE_DIR}/src/page_faults/page_fault_analysis.cpp)
target_include_directories(page_fault_analysis PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)