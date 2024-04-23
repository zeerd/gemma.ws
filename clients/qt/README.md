# Gemma.QT

## Functions

* 本地聊天机器人；
* Local Chatbot Using Gemma-2B.

* 总结完整的纯文本文件的内容（很慢）；
* Parse a whole plaintext file/source-code for summary(slow).

* 以函数为单位逐个解析，速度相对快一些，但是精度下降；
* Parse C/C++ source file function by function for speed up.

* 以 Markdown 格式保存聊天过程；
* Save the conversation as a Markdown file.

## Run

* libGemmaCore 被链接到 Gemma.QT 上了，以便它可以脱离 ServerGemma 独立运行。
* We link the libGemmaCore into the Gemma.QT,
  so that it could run independently without ServerGemma.

* 可以通过菜单 `Edit->Setting` 加载权重文件。
* Select weight-files by `Edit->Setting`.

### Debug

* [调试](https://doc.qt.io/qt-6/qtwebengine-debugging.html) WebEngine 的方法。
Way to [debug](https://doc.qt.io/qt-6/qtwebengine-debugging.html) the WebEngine.

```bash
./build/clients/qt/Gemma.QT --remote-debugging-port=12345
```
