cmake_minimum_required(VERSION 3.10)
project(pipe_spawn C)

# Export compile_commands.json for clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(pipe_spawn pipe_main.c)
