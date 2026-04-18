cmake_minimum_required(VERSION 3.15)
project(DebugTool)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add subdirectories for libraries
add_subdirectory(Utilities)
add_subdirectory(DebugClient)
add_subdirectory(DebugServer)

# Main executable
add_executable(DebugTool main.cpp)

target_link_libraries(DebugTool
    PRIVATE
        DebugClient
        DebugServer
        Utilities
)

# Include directories for main
target_include_directories(DebugTool
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        DebugClient/ClientPublic/inc
        DebugClient/ClientInternal/ClientProtocol/inc
        DebugClient/ClientInternal/ClientTransport/inc
        DebugServer/ServerPublic/inc
        DebugServer/ServerInternal/OpenCD/inc
        DebugServer/Interface
)
