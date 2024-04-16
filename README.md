# Gemma.QT

* 基于QT图形框架的 Gemma 聊天机器人。
* Gemma with QT UI Framework.

* 开发中……
* Developing...

## Functions

* 本地聊天机器人；
* Local Chatbot Using Gemma-2B.

* 总结完整的纯文本文件的内容（很慢）；
* Parse a whole plaintext file/source-code for summary(slow).

* 以函数为单位逐个解析，速度相对快一些，但是精度下降；
* Parse C/C++ source file function by function for speed up.

* 以 Markdown 格式保存聊天过程；
* Save the conversation as a Markdown file.

* 基于 WebSocket 的 CodeGemma Sublime Text 编程助手；
* Coding assistant based on Code Gemma for Sublime Text.

## Build

1. Install packages

```bash
sudo apt install build-essential cmake
sudo apt install git libtcmalloc-minimal4 zlib1g-dev
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
cmake .. -DBUILD_TESTING=OFF -DSPM_ENABLE_SHARED=OFF
make -j libgemma
cd -

qmake
make -j
```

Note:
If you want to use the weight-file without `sfp`,
add `-DWEIGHT_TYPE=hwy::bfloat16_t` to `cmake` command.

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
