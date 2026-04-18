#!/bin/bash

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"
EXECUTABLE="${BUILD_DIR}/DebugTool"

echo "=== DebugTool Build and Run Script ==="
echo "Script directory: $SCRIPT_DIR"
echo "Build directory: $BUILD_DIR"
echo ""

# Create build directory if it doesn't exist
if [ ! -d "$BUILD_DIR" ]; then
    echo "[*] Creating build directory..."
    mkdir -p "$BUILD_DIR"
fi

# Navigate to build directory
cd "$BUILD_DIR"

# Run CMake
echo "[*] Running CMake..."
cmake ..

# Build the project
echo "[*] Building project..."
cmake --build . --config Release -j"$(nproc)"

echo ""
echo "=== Build Complete ==="

# Check if executable exists
if [ -f "$EXECUTABLE" ]; then
    echo "[✓] Executable created: $EXECUTABLE"
    echo ""
    echo "To run the executable:"
    echo "  $EXECUTABLE"
else
    echo "[!] Warning: Executable not found at $EXECUTABLE"
    exit 1
fi
