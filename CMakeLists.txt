cmake_minimum_required(VERSION 3.5)

if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
    message(FATAL_ERROR "in-source builds are not allowed")
endif()

project(gate_sim)
set(CMAKE_CXX_STANDARD 17)

# Enable address sanitizer.
if(UNIX)
    SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
    SET(CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
endif()

set(GATE_SIM_SOURCES
    src/main.cc

    extern/bas/src/aligned_allocation.cc
)

include_directories(
    extern/bas/include/
)

add_executable(gate_sim ${GATE_SIM_SOURCES})

# Generate many warnings.
if(MSVC)
  target_compile_options(gate_sim PRIVATE /W4 /WX)
else()
  target_compile_options(gate_sim PRIVATE -Wall -Wextra -pedantic -Werror)
endif()
