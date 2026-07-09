cmake_minimum_required(VERSION 3.10)
project(fifo_named_pipe C)

# Export compile_commands.json for clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(fifo_named_pipe_p1 fifo_named_pipe_p1.c)
add_executable(fifo_named_pipe_p2 fifo_named_pipe_p2.c)
