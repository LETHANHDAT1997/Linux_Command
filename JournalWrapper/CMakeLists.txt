cmake_minimum_required(VERSION 3.10)
project(JournalWrapper)

# Bật C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Thêm file nguồn
add_executable(JournalWrapper main.cpp JournalWrapper.cpp)

# Phải gọi add_executable trước khi sử dụng target_include_directories
target_include_directories(JournalWrapper PRIVATE
    ${CMAKE_SOURCE_DIR}
)

# Link thư viện yaml-cpp
target_link_libraries(JournalWrapper systemd)
