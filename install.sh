#!/usr/bin/env bash

if [ ! -d "./build" ]; then
    mkdir ./build
fi

cd ./build

cmake ..
make

sudo mv ./download /usr/bin/ani-download
