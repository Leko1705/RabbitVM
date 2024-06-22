cmake_minimum_required(VERSION 3.26)
project(RabbitVM C)
enable_language(ASM_NASM)

set(CMAKE_C_STANDARD 11)

set_source_files_properties(${ASM_SOURCES} PROPERTIES LANGUAGE ASM_NASM)

add_executable(RabbitVM main.c
        utils.h
        load.c
        load.h
        thread.h
        thread.c
        pool.h
        pool.c
        env.h
        env.c
        utils.c
        rni.c
        rni.h
)
