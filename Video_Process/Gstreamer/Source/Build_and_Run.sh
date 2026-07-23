#!/bin/bash

# Thiết lập chế độ thoát ngay lập tức nếu gặp lỗi
set -e

# Lấy thư mục chứa script
SOURCE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SOURCE_DIR"

FILE1="Build_PipeLine_Elements.c"
FILE2="Build_PipeLine_Parse.c"
FILE3="Scan_Cameras.c"
FILE4="Camera_Devices.cpp"
FILE5="Build_Live_Preview.c"
OUT1="Build_PipeLine_Elements"
OUT2="Build_PipeLine_Parse"
OUT3="Scan_Cameras"
OUT4="Camera_Devices"
OUT5="Build_Live_Preview"
BUILD_FOLDER="build"

build_and_run() 
{
    # Kiểm tra thư mục build đã tồn tại hay chưa
    if [ ! -d "$BUILD_FOLDER" ]; then
        mkdir "$BUILD_FOLDER"
    fi

    local src_file=$1
    local out_bin=$2

    echo "=============================================="
    echo "🔨 Đang biên dịch $src_file..."
    echo "=============================================="
    
    # Kiểm tra xem gói GStreamer dev đã được cài đặt chưa
    if ! pkg-config --exists gstreamer-1.0; then
        echo "❌ Lỗi: Không tìm thấy thư viện gstreamer-1.0 qua pkg-config."
        echo "Vui lòng cài đặt thư viện phát triển GStreamer trước:"
        echo "  sudo apt install libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev libglib2.0-dev"
        exit 1
    fi

    # Biên dịch chương trình và đặt file thực thi vào thư mục build
    if gcc -o "$BUILD_FOLDER/$out_bin" "$src_file" $(pkg-config --cflags --libs gstreamer-1.0); then
        echo "✅ Biên dịch thành công!"
        echo "=============================================="
        echo "🚀 Đang khởi chạy chương trình $BUILD_FOLDER/$out_bin..."
        echo "=============================================="
        ./$BUILD_FOLDER/$out_bin
    else
        echo "❌ Biên dịch thất bại!"
        exit 1
    fi
}

build_and_run_cpp() 
{
    # Kiểm tra thư mục build đã tồn tại hay chưa
    if [ ! -d "$BUILD_FOLDER" ]; then
        mkdir "$BUILD_FOLDER"
    fi

    local src_file=$1
    local out_bin=$2
    shift 2
    local app_args="$@"

    echo "=============================================="
    echo "🔨 Đang biên dịch (C++) $src_file..."
    echo "=============================================="
    
    # Kiểm tra xem gói GStreamer dev đã được cài đặt chưa
    if ! pkg-config --exists gstreamer-1.0; then
        echo "❌ Lỗi: Không tìm thấy thư viện gstreamer-1.0 qua pkg-config."
        echo "Vui lòng cài đặt thư viện phát triển GStreamer trước:"
        echo "  sudo apt install libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev libglib2.0-dev"
        exit 1
    fi

    # Biên dịch chương trình C++ bằng g++ và đặt file thực thi vào thư mục build
    if g++ -std=c++17 -o "$BUILD_FOLDER/$out_bin" "$src_file" $(pkg-config --cflags --libs gstreamer-1.0 gstreamer-app-1.0); then
        echo "✅ Biên dịch thành công!"
        echo "=============================================="
        echo "🚀 Đang khởi chạy chương trình $BUILD_FOLDER/$out_bin $app_args..."
        echo "=============================================="
        ./$BUILD_FOLDER/$out_bin $app_args
    else
        echo "❌ Biên dịch thất bại!"
        exit 1
    fi
}

show_menu() {
    echo "Chọn tệp mã nguồn GStreamer để biên dịch và chạy:"
    echo "1) $FILE1 (Khởi tạo Pipeline bằng Elements thủ công)"
    echo "2) $FILE2 (Khởi tạo Pipeline bằng Parse chuỗi ký tự)"
    echo "3) $FILE3 (Quét thiết bị Camera kết nối - C)"
    echo "4) $FILE4 (Live Preview & Sink Config - C++)"
    echo "5) $FILE5 (Xem Live Preview từ Camera bằng OpenGL)"
    echo "6) Thoát"
    read -p "Nhập lựa chọn của bạn [1-6]: " choice
    case $choice in
        1)
            build_and_run "$FILE1" "$OUT1"
            ;;
        2)
            build_and_run "$FILE2" "$OUT2"
            ;;
        3)
            build_and_run "$FILE3" "$OUT3"
            ;;
        4)
            echo ""
            echo "  Chọn chế độ Test cho $FILE4:"
            echo "  1) TEST 1: glimagesink  (Live Preview OpenGL - Ctrl+C để thoát)"
            echo "  2) TEST 2: fakesink     (Headless - Ctrl+C để thoát)"
            echo "  3) TEST 3: appsink      (Frame Callback - tự thoát sau 150 frame)"
            read -p "  Nhập lựa chọn test [1-3] (mặc định 1): " test_choice
            test_choice=${test_choice:-1}
            build_and_run_cpp "$FILE4" "$OUT4" "$test_choice"
            ;;
        5)
            build_and_run "$FILE5" "$OUT5"
            ;;
        6)
            echo "Đang thoát."
            exit 0
            ;;
        *)
            echo "Lựa chọn không hợp lệ!"
            echo ""
            show_menu
            ;;
    esac
}

# Kiểm tra tham số dòng lệnh trước để cho phép chạy nhanh
if [ "$1" == "1" ] || [ "$1" == "elements" ]; then
    build_and_run "$FILE1" "$OUT1"
elif [ "$1" == "2" ] || [ "$1" == "parse" ]; then
    build_and_run "$FILE2" "$OUT2"
elif [ "$1" == "3" ] || [ "$1" == "list" ] || [ "$1" == "monitor" ]; then
    build_and_run "$FILE3" "$OUT3"
elif [ "$1" == "4" ] || [ "$1" == "camera" ] || [ "$1" == "cpp" ]; then
    build_and_run_cpp "$FILE4" "$OUT4" "$2"
elif [ "$1" == "5" ] || [ "$1" == "preview" ] || [ "$1" == "live" ]; then
    build_and_run "$FILE5" "$OUT5"
else
    show_menu
fi

