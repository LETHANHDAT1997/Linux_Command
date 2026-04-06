cmake_minimum_required(VERSION 3.18)
project(STM32Debugger CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Threads REQUIRED)

set(SOURCES
    src/DebugCore.cpp
    src/protocol/GDBMIParser.cpp
    src/protocol/GDBMIWriter.cpp
    src/protocol/GDBMIReader.cpp
    src/protocol/GDBMIChannel.cpp
    src/process/ProcessHandle.cpp
    src/process/GDBProcess.cpp
    src/process/OpenOCDProcess.cpp
    src/adapter/BreakpointManager.cpp
    src/adapter/VariableManager.cpp
    src/adapter/DebugStateMachine.cpp
    src/adapter/EventDispatcher.cpp
    src/adapter/GDBDebugger.cpp
)

add_library(stm32debugger STATIC ${SOURCES})
target_include_directories(stm32debugger PUBLIC include)
target_link_libraries(stm32debugger Threads::Threads)
target_compile_options(stm32debugger PRIVATE
    -Wall -Wextra -Wpedantic
    -Wno-unused-parameter
)

# Optional: demo/test executable
add_executable(debugger_demo main_demo.cpp)
target_link_libraries(debugger_demo stm32debugger)
