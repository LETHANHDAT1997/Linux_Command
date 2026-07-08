#!/bin/bash
if [ ! -d build ]; then
    mkdir -p build
fi

cd build || exit

cmake ..
make

echo "-------------------------------------------------------"
echo "Build complete!"
echo "De chay kiem thu:"
echo "1. Mo mot Terminal moi va chay Server:"
echo "   ./build/socket_default_server"
echo "2. Mo mot Terminal khac va chay Client:"
echo "   ./build/socket_default_client"
echo "3. Thử gia lập user khac (hoac dung sudo neu server khong chay quyen root) de test filter:"
echo "   sudo ./build/socket_default_client"
echo "-------------------------------------------------------"