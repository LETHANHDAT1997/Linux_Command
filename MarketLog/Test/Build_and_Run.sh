#!/bin/bash

# Kiểm tra xem thư mục hiện tại có chứa file CMakeLists.txt
if [ ! -f "CMakeLists.txt" ]; then
    echo "Không tìm thấy CMakeLists.txt trong thư mục hiện tại."
    exit 1
fi

# Kiểm tra xem thư mục build có tồn tại chưa, nếu chưa thì tạo mới
if [ ! -d "build" ]; then
    echo "Tạo thư mục build..."
    mkdir build
fi

# Di chuyển vào thư mục build
cd build

# Chạy CMake từ thư mục build và chỉ định thư mục gốc của dự án là ..
echo "Đang chạy CMake từ thư mục build..."
# cmake ..
cmake .. -G Ninja

# Kiểm tra kết quả của CMake và tiến hành biên dịch nếu thành công
if [ $? -eq 0 ]; then
    echo "CMake thành công. Đang biên dịch dự án..."
    # make
    ninja
else
    echo "CMake gặp lỗi. Dự án không thể biên dịch."
    exit 1
fi

# cd ..
# ./build/MarKetLog