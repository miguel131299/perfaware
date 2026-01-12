# Sim8086 Project - 8086 CPU Simulator

set(SIM8086_HEADERS 
      ${CMAKE_CURRENT_SOURCE_DIR}/include/sim8086/decoder.hpp
      ${CMAKE_CURRENT_SOURCE_DIR}/include/sim8086/utils.hpp
      ${CMAKE_CURRENT_SOURCE_DIR}/include/sim8086/instruction_cycles.hpp
      ${CMAKE_CURRENT_SOURCE_DIR}/include/sim8086/simulator.hpp
)

set(SIM8086_SOURCES 
      ${CMAKE_CURRENT_SOURCE_DIR}/src/decoder/decoder.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/src/decoder/operand_decoder.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/src/decoder/instructions/generic/generic_regmem_handler.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/src/decoder/instructions/jumps/generic_jump_handler.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/src/decoder/instructions/arithmetic/arithmetic_immediate_accumulator_handler.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/src/decoder/instructions/arithmetic/arithmetic_immediate_regmem_handler.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/src/decoder/instructions/arithmetic/immediate_regmem_group_handler.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/src/decoder/instructions/mov/mov_immediate_handler.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/src/decoder/instructions/mov/mov_immediate_regmem_handler.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/src/decoder/instructions/mov/mov_mem_accumulator_handler.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/src/decoder/instructions/mov/mov_accumulator_mem_handler.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/src/simulator/simulator.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/src/instruction_cycles.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/src/utils/utils.cpp
)

# Core library
add_library(sim8086_core STATIC ${SIM8086_SOURCES})
target_include_directories(sim8086_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(sim8086_core PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/decoder)
target_include_directories(sim8086_core PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/simulator)

# Main Binary
add_executable(sim8086 ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)
target_link_libraries(sim8086 PRIVATE sim8086_core)
target_include_directories(sim8086 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Tests
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/tests)
