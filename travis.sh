#!/bin/bash

sudo apt update

sudo apt install -y build-essential cmake git libtcmalloc-minimal4 zlib1g-dev libgoogle-perftools-dev
sudo apt install -y libqt5webview5-dev libqt5webchannel5-dev qtwebengine5-dev libqt5websockets5-dev

git submodule init
git submodule update

cd 3rdparty/gemma.cpp/build
cmake .. -DBUILD_TESTING=OFF -DSPM_ENABLE_SHARED=OFF
# cmake .. -DWEIGHT_TYPE=hwy::bfloat16_t -DBUILD_TESTING=OFF -DSPM_ENABLE_SHARED=OFF
make -j4
cd -

cd build
cmake ..
make -j4
cd -

qmake
make -j4

./gemma
