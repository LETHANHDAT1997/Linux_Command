add_library(DebugClient STATIC
    ClientPublic/src/DebugClient.cpp
    ClientPublic/inc/DebugClient.h
    ClientInternal/ClientProtocol/src/GDBProtocol.cpp
    ClientInternal/ClientProtocol/inc/GDBProtocol.h
    ClientInternal/ClientTransport/src/TCPTransport.cpp
    ClientInternal/ClientTransport/inc/TCPTransport.h
    Interface/Protocol/IProtocol.h
    Interface/Transport/ITransport.h
)

target_include_directories(DebugClient
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/ClientPublic/inc
        ${CMAKE_CURRENT_SOURCE_DIR}/Interface/Protocol
        ${CMAKE_CURRENT_SOURCE_DIR}/Interface/Transport
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/ClientInternal/ClientProtocol/inc
        ${CMAKE_CURRENT_SOURCE_DIR}/ClientInternal/ClientTransport/inc
)

target_link_libraries(DebugClient
    PUBLIC
        Utilities
)
