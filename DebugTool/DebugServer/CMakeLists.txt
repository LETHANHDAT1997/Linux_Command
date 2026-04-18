add_library(DebugServer STATIC
    ServerPublic/src/DebugServer.cpp
    ServerPublic/inc/DebugServer.h
    ServerInternal/OpenCD/src/OpenOCD.cpp
    ServerInternal/OpenCD/inc/OpenOCD.h
    Interface/IDebugServer.h
)

target_include_directories(DebugServer
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/ServerPublic/inc
        ${CMAKE_CURRENT_SOURCE_DIR}/Interface
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/ServerInternal/OpenCD/inc
)

target_link_libraries(DebugServer
    PUBLIC
        Utilities
)
