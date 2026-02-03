#!/bin/bash

# 1. Setup paths
export PICO_SDK_PATH="/home/oleh/pico/pico-sdk"

# 2. Clean start
rm -rf build
mkdir build
cd build

# 3. Run CMake with explicit System paths for the ARM toolchain
cmake -DCMAKE_C_COMPILER=/usr/bin/arm-none-eabi-gcc \
      -DCMAKE_CXX_COMPILER=/usr/bin/arm-none-eabi-g++ \
      -DCMAKE_ASM_COMPILER=/usr/bin/arm-none-eabi-gcc \
      -DPICOTOOL_FETCH_FROM_GIT=OFF \
      ..

# 4. Build the project
make -j$(nproc)
