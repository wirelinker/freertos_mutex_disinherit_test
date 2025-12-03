#!/bin/bash

# Exit on error
set -e

read -n 10 -r -p"Enter pico board type:(pico, pico_w, pico2, pico2_w) " BOARD

read -n 15 -r -p"Set build type of binary (Debug, Release, MinSizeRel, RelWithDebInfo): " BUILD_TYPE

echo "The board is \"${BOARD}\". The build type is \"${BUILD_TYPE}\"."

read -n 1 -r -p"If it's correct, press enter."

cmake -DCMAKE_BUILD_TYPE=${BUILD_TYPE} -DPICO_BOARD=${BOARD} ..

