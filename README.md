
1. Install QT5

```bash
sudo apt install build-essential cmake
sudo apt install libqt5webview5-dev libqt5webchannel5-dev qtwebengine5-dev
sudo apt install git libtcmalloc-minimal4
```

2. Compile project.

```bash
git clone https://github.com/zeerd/gemma.qt
cd gemma.qt
git submodule init
git submodule update

cd gemma.cpp/build
cmake .. -DWEIGHT_TYPE=hwy::bfloat16_t -DBUILD_TESTING=OFF -DSPM_ENABLE_SHARED=OFF
make -j4 gemma
cd -

qmake
make
```

3. Run

Visit [the Gemma model page on
Kaggle](https://www.kaggle.com/models/google/gemma) and
select `Model Variations|> Gemma C++`. Download the `2b-it-sfp` model.

```bash
tar -xf archive.tar.gz
./gemma
```
