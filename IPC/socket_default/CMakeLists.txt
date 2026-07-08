cmake_minimum_required(VERSION 3.10)
project(socket_default C)

# Export compile_commands.json for clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(socket_default_server socket_default_server.c)
add_executable(socket_default_client socket_default_client.c)
