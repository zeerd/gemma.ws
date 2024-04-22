# Gemma.WS

The core of this repo is ServerGemma.
It's a Gemma server basic on
[WebSocket](https://github.com/machinezone/IXWebSocket),
[json](https://github.com/nlohmann/json.git)
and [GemmaC++](https://github.com/google/gemma.cpp).

本工程的核心部分是 `ServerGemma`。
基于[WebSocket](https://github.com/machinezone/IXWebSocket) 、
[json](https://github.com/nlohmann/json.git)
和 [GemmaC++](https://github.com/google/gemma.cpp) 搭建而成。

## Gemma.QT

* Gemma.QT is one of the developing client based on QT UI Framework.
  * Gemma.QT 是基于QT图形框架的 Gemma 聊天机器人客户端。

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

## Build & Run

1. Prepare packages and Compile project.

Visit [the Gemma model page on
Kaggle](https://www.kaggle.com/models/google/gemma) and
select `Model Variations|> Gemma C++`.
Download the `2b-it-sfp` model and decompress.

Run `./travis.sh` to compile and run.

Note:
If you want to use the weight-file without `sfp`,
add `-DWEIGHT_TYPE=hwy::bfloat16_t` to `cmake` command.

Select weight-files by `Edit->Setting`.

2. Debug

Way to [debug](https://doc.qt.io/qt-6/qtwebengine-debugging.html) the WebEngine.

```bash
./gemma --remote-debugging-port=12345
```
