cmake_minimum_required (VERSION 3.10)

project (fanctrl)

set (CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror")
set (source_dir "${PROJECT_SOURCE_DIR}/src/")

file (GLOB source_files "${source_dir}/*.c")

add_executable (fanctrl ${source_files})
