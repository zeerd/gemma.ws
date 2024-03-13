# Gemma.QT

Gemma with QT UI Framework.

## Functions

1. Local Chatbot Using Gemma-2B.

2. Parse a whole plaintext file/source-code for summary(slow).

3. Parse C/C++ source file function by function for speed up.

4. Save the conversation as a Markdown file.

## Build

1. Install packages

```bash
sudo apt install build-essential cmake
sudo apt install git libtcmalloc-minimal4
```

```bash
sudo apt install libqt5webview5-dev libqt5webchannel5-dev qtwebengine5-dev
```

2. Compile project.

```bash
git clone https://github.com/zeerd/gemma.qt
cd gemma.qt
git submodule init
git submodule update

cd gemma.cpp/build
cmake .. -DWEIGHT_TYPE=hwy::bfloat16_t -DBUILD_TESTING=OFF -DSPM_ENABLE_SHARED=OFF
make -j libgemma
cd -

qmake
make -j
```

3. Run

Visit [the Gemma model page on
Kaggle](https://www.kaggle.com/models/google/gemma) and
select `Model Variations|> Gemma C++`.
Download the `2b-it-sfp` model and decompress.

```bash
./gemma
```

Select decompressed files by `Edit->Setting`.

4. Debug

Way to [debug](https://doc.qt.io/qt-6/qtwebengine-debugging.html) the WebEngine.

```bash
./gemma --remote-debugging-port=12345
```
