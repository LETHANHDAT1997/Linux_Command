#!/bin/bash
if [ ! -d build ]; then
    mkdir -p build
fi

cd build || exit

cmake ..
make

# ./fifo_named_pipe_p1