# Performance Testing Tools Project

# Conditional NOP loops test executable (with ASM)
set(CONDITIONAL_NOP_ASM_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/src/tools/conditional_nop_loop.asm)
set(CONDITIONAL_NOP_ASM_OBJECT ${CMAKE_CURRENT_BINARY_DIR}/conditional_nop_loop.o)

add_custom_command(
  OUTPUT ${CONDITIONAL_NOP_ASM_OBJECT}
  COMMAND nasm -f elf64 ${CONDITIONAL_NOP_ASM_SOURCE} -o ${CONDITIONAL_NOP_ASM_OBJECT}
  DEPENDS ${CONDITIONAL_NOP_ASM_SOURCE}
  COMMENT "Assembling conditional_nop_loop.asm with NASM"
)

add_executable(conditional_nop_loops_tester
  ${CMAKE_CURRENT_SOURCE_DIR}/src/tools/conditional_nop_loops_main.cpp
  ${CONDITIONAL_NOP_ASM_OBJECT}
)
target_include_directories(conditional_nop_loops_tester PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Multinop loops test executable (with ASM)
set(MULTINOP_ASM_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/src/tools/multinop_loops.asm)
set(MULTINOP_ASM_OBJECT ${CMAKE_CURRENT_BINARY_DIR}/multinop_loops.o)

add_custom_command(
  OUTPUT ${MULTINOP_ASM_OBJECT}
  COMMAND nasm -f elf64 ${MULTINOP_ASM_SOURCE} -o ${MULTINOP_ASM_OBJECT}
  DEPENDS ${MULTINOP_ASM_SOURCE}
  COMMENT "Assembling multinop_loops.asm with NASM"
)

add_executable(multinop_loops_tester
  ${CMAKE_CURRENT_SOURCE_DIR}/src/tools/multinop_loops_main.cpp
  ${MULTINOP_ASM_OBJECT}
)
target_include_directories(multinop_loops_tester PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Repetition tester executable
add_executable(repetition_tester ${CMAKE_CURRENT_SOURCE_DIR}/src/tools/repetition_test_main.cpp)
target_include_directories(repetition_tester PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Frontend test executable (with ASM)
set(ASM_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/src/tools/nop_loop.asm)
set(ASM_OBJECT ${CMAKE_CURRENT_BINARY_DIR}/nop_loop.o)

# Custom command to assemble the .asm file with NASM
add_custom_command(
  OUTPUT ${ASM_OBJECT}
  COMMAND nasm -f elf64 ${ASM_SOURCE} -o ${ASM_OBJECT}
  DEPENDS ${ASM_SOURCE}
  COMMENT "Assembling nop_loop.asm with NASM"
)

add_executable(frontend_tester 
  ${CMAKE_CURRENT_SOURCE_DIR}/src/tools/frontend_test_main.cpp
  ${ASM_OBJECT}
)
target_include_directories(frontend_tester PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Page fault test executable
add_executable(page_fault_tester ${CMAKE_CURRENT_SOURCE_DIR}/src/page_faults/page_fault_test_main.cpp)
target_include_directories(page_fault_tester PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Pointer analysis executable
add_executable(pointer_analyzer ${CMAKE_CURRENT_SOURCE_DIR}/src/page_faults/pointer_analysis_main.cpp)
target_include_directories(pointer_analyzer PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Page fault analysis executable
add_executable(page_fault_analysis ${CMAKE_CURRENT_SOURCE_DIR}/src/page_faults/page_fault_analysis.cpp)
target_include_directories(page_fault_analysis PRIVATE ${CMAKE_SOURCE_DIR}/include)
