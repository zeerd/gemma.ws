#!/bin/bash

sudo apt update

sudo apt install -y build-essential cmake git libtcmalloc-minimal4 zlib1g-dev libgoogle-perftools-dev
sudo apt install -y libqt5webview5-dev libqt5webchannel5-dev qtwebengine5-dev libqt5websockets5-dev
sudo apt install -y libmagick++-dev

git submodule init
git submodule update

set -e

cd build
cmake .. -DBUILD_TESTING=OFF -DSPM_ENABLE_SHARED=OFF
make -j4 Gemma.QT ServerGemma
cd -

./build/clients/qt/Gemma.QT
