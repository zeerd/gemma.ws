# Gemma.WS

[![Ask DeepWiki](https://deepwiki.com/badge.svg)](https://deepwiki.com/zeerd/gemma.ws)

The core of this repo is [ServerGemma](./server/README.md).
It's a Gemma server based on
[WebSocket](https://github.com/machinezone/IXWebSocket),
[json](https://github.com/nlohmann/json.git)
and [GemmaC++](https://github.com/google/gemma.cpp).

本工程的核心部分是 [ServerGemma](./server/README.md)。
基于[WebSocket](https://github.com/machinezone/IXWebSocket) 、
[json](https://github.com/nlohmann/json.git)
和 [GemmaC++](https://github.com/google/gemma.cpp) 搭建而成。

## Functions

* [Gemma.QT](./clients/qt/README.md) is one of the developing client based on QT UI Framework.
  * [Gemma.QT](./clients/qt/README.md) 是基于QT图形框架的 Gemma 聊天机器人客户端。

* [Coding assistant](./clients/CodeGamme-SublimeTextPlugin/README.md) based on Code Gemma for Sublime Text.
  * 基于 WebSocket 的 CodeGemma Sublime Text [编程助手](./clients/CodeGamme-SublimeTextPlugin/README.md)；

* Developing...
  * 开发中……

## Build & Run

Visit [the Gemma model page on
Kaggle](https://www.kaggle.com/models/google/gemma) and
select `Model Variations|> Gemma C++`.
Download the `2b-it-sfp` model and decompress.

Run `./travis.sh` to compile and run.

Note:
If you want to use the weight-file without `sfp`,
add `-DWEIGHT_TYPE=hwy::bfloat16_t` to `cmake` command.
