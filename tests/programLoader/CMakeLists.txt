cmake_minimum_required(VERSION 2.6 FATAL_ERROR)

add_subdirectory(src)

add_test( ProgramLoader src/testProgramLoader  "${PROJECT_SOURCE_DIR}/tests/programLoader/config/program1.rul" "${PROJECT_SOURCE_DIR}/tests/programLoader/data/input.txt" )
