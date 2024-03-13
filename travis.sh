#!/bin/bash

sudo apt update

sudo apt install -y build-essential cmake git libtcmalloc-minimal4
sudo apt install -y libqt5webview5-dev libqt5webchannel5-dev qtwebengine5-dev

git submodule init
git submodule update

cd gemma.cpp/build
cmake .. -DWEIGHT_TYPE=hwy::bfloat16_t -DBUILD_TESTING=OFF -DSPM_ENABLE_SHARED=OFF
make -j4 libgemma
cd -

qmake
make -j4
