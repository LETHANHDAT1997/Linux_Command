cmake_minimum_required(VERSION 3.10)
project(MarKetLog)

# Bật C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_definitions(-DYAML_CPP_STATIC_DEFINE)

# Thêm file nguồn
add_executable(MarKetLog 
    main.cpp 
    MarketLogRotator.cpp
    LogWatcher.cpp
    MarKetLog.cpp 
    MarKetLogUtility.cpp
    MarKetLogConfig.cpp
    /home/ledat/Downloads/MarKetLog/JKC_Lib/JKEvent.cpp
    /home/ledat/Downloads/MarKetLog/JKC_Lib/JKSync.cpp
    /home/ledat/Downloads/MarKetLog/JKC_Lib/JKStorage.cpp
)

# Phải gọi add_executable trước khi sử dụng target_include_directories
target_include_directories(MarKetLog PRIVATE
    ${CMAKE_SOURCE_DIR}/*.h
    /home/ledat/Downloads/MarKetLog/JKC_Lib
    /home/ledat/Downloads/MarKetLog/install/include
)

# Tìm và khai báo thư viện Threads
find_package(Threads REQUIRED)

# Link thư viện yaml-cpp
target_link_libraries(MarKetLog /home/ledat/Downloads/MarKetLog/install/lib/libyaml-cpp.a)
target_link_libraries(MarKetLog Threads::Threads)
