# ServerGemma

## About API

* I am using the [OpenAI API](https://platform.openai.com/docs/api-reference/chat/streaming) to be the basic API spec.
* 我参照了[OpenAI API](https://platform.openai.com/docs/api-reference/chat/streaming)作为基本的API。

## Conf

* I am using the Result of `QSetting` as the configuration file.
  So, If you are using the `ServerGemma` only, you can generate a configuration
  file first by the QT version or just wrote it manually. Like below:
* 我使用了 QSetting 的结果作为配置文件。
  如果你打算直接使用`ServerGemma`，可以考虑使用`QT`版本的`gemma`先生成一次配置
  文件，或者直接手写一份。参照下面示例：

```ini
[Setting]
MaxGeneratedTokens=2048
MaxTokens=3072
Temperature=1.0
Verbosity=2
ModelType=2b-pt
Weight=/opt/gemma/CodeGemma/2b-pt-sfp.sbs
Tokenizer=/opt/gemma/CodeGemma/tokenizer.spm
WebSocketPort=9999
```
