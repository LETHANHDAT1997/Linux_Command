cmake_minimum_required(VERSION 3.10)
project(MarKetLog)

# Bật C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Thêm file nguồn
add_executable(MarKetLog main.cpp MarketLogRotator.cpp MarKetLog.cpp MarKetLogFileHandle.cpp)

# Phải gọi add_executable trước khi sử dụng target_include_directories
target_include_directories(MarKetLog PRIVATE
    ${CMAKE_SOURCE_DIR}
)

# Link thư viện yaml-cpp
# target_link_libraries(MarKetLog ${CMAKE_SOURCE_DIR}/install/lib/libyaml-cpp.a)
target_link_libraries(MarKetLog PRIVATE stdc++fs)
