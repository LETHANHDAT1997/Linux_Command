cmake_minimum_required(VERSION 3.12)
project(DebugTool VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic")

# Include directories
include_directories(${PROJECT_SOURCE_DIR}/inc)

# Source files
set(SOURCES
    src/main.cpp
    src/DebugCore.cpp
    src/GDBDebugger.cpp
    src/GDBMIProtocol.cpp
    src/ProcessHandle.cpp
)

# Header files
set(HEADERS
    inc/DebugCore.h
    inc/DebugStateMachine.h
    inc/EventDispatcher.h
    inc/GDBDebugger.h
    inc/GDBMIChannel.h
    inc/GDBMIParser.h
    inc/GDBMIReader.h
    inc/GDBMIWriter.h
    inc/IDebugEventListener.h
    inc/IDebugger.h
    inc/MIRecord.h
    inc/ProcessHandle.h
    inc/Types.h
    inc/VariableManager.h
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Link required libraries
target_link_libraries(${PROJECT_NAME}
    PRIVATE
    pthread
)

# Set output directory
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Optional: Enable testing
enable_testing()
