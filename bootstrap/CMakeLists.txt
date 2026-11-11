cmake_minimum_required(VERSION 3.20)
project(son0 C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

include_directories(include)

set(SOURCES
    src/son_common.c
    src/son_lexer.c
    src/son_parser.c
    src/son_sema.c
    src/son_ir.c
    src/son_ai.c
    src/son_codegen.c
    src/main.c
)

add_executable(son0 ${SOURCES})
