add_library(Utilities STATIC
    ProcessHandle.cpp
    ProcessHandle.h
)

target_include_directories(Utilities
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link system libraries
target_link_libraries(Utilities
    PRIVATE
        pthread
)
