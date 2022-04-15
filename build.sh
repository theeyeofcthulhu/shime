#!/usr/bin/env bash

set -e

if [[ $1 == "clean" ]]; then
    rm -rf ./build
fi

mkdir -p build
cd build

cmake ..
make

cd ..
cp -v build/shime .
cp -v build/compile_commands.json .
