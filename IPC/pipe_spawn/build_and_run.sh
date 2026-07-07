#!/bin/bash
if [ ! -d build ]; then
    mkdir -p build
fi

cd build || exit

cmake ..
make

./pipe_spawn