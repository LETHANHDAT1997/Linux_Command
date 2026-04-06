#!/bin/bash

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Check if CMakeLists.txt exists in current directory
if [ ! -f "CMakeLists.txt" ]; then
    echo -e "${RED}Lỗi: Không tìm thấy CMakeLists.txt trong thư mục hiện tại.${NC}"
    exit 1
fi

# Create build directory if it doesn't exist
if [ ! -d "build" ]; then
    echo -e "${YELLOW}Tạo thư mục build...${NC}"
    mkdir build
fi

# Change to build directory
cd build

# Run CMake
echo -e "${YELLOW}Đang chạy CMake...${NC}"
cmake .. -G Unix\ Makefiles

# Check if CMake succeeded
if [ $? -ne 0 ]; then
    echo -e "${RED}Lỗi: CMake gặp lỗi. Dự án không thể biên dịch.${NC}"
    exit 1
fi

# Compile the project
echo -e "${YELLOW}Đang biên dịch dự án...${NC}"
make -j$(nproc)

# Check if compilation succeeded
if [ $? -ne 0 ]; then
    echo -e "${RED}Lỗi: Biên dịch gặp lỗi.${NC}"
    exit 1
fi

echo -e "${GREEN}Biên dịch thành công!${NC}"

# Run the executable
if [ -f "bin/DebugTool" ]; then
    echo -e "${YELLOW}Đang chạy DebugTool...${NC}"
    ./bin/DebugTool
else
    echo -e "${RED}Lỗi: Không tìm thấy executable bin/DebugTool${NC}"
    exit 1
fi
