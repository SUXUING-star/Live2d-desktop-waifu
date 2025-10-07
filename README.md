# 虚拟Live2d交互对话项目（自用demo）

这是一个基于 Live2D Cubism Native SDK 开发的桌面虚拟角色项目。

## 特性

*   **实时对话：** 基于 Ollama 的本地大语言模型。
*   **语音合成：** 使用 GPTSOVITS，声音可以自行选择。
*   **表情动作联动：** AI 可以控制模型的表情和动作，让她“活”起来。

## 运行环境

*  支持 OpenGL 的显卡
*  大语言模型的api接口与声音生成的api接口

## 如何编译

本项目**不包含** Live2D Cubism Native SDK 的源代码。你需要先从 Live2D 官网下载，然后再把本仓库的代码放进去。

1.  **下载 Cubism SDK:**
    *   访问 [Live2D Cubism 官网](https://www.live2d.com/en/sdk/download/cubism/)

2.  **克隆本项目:**
    *   把这个仓库克隆到 `<SDK_ROOT>/Samples/OpenGL/` 目录下，并命名为 `Demo` (或者你喜欢的名字)。

3.  **下载并配置依赖**
    *  需要安装以下依赖imgui,nlohmann_json ,cpr, ffmpeg,miniaudio放入libs文件夹下

4.  **编译:**
    *   对项目文件的cmakelists进行编译后，在 Visual Studio 中打开生成的 `.sln` 文件，编译并运行。

## 注意事项

*   你需要自行部署并运行 Ollama 和 GPTSOVITS API 服务，并确保 C++ 代码里的 API 地址是正确的。
*   模型文件在 `Resources/` 文件夹下，你可以替换成你自己的模型。

## 许可证

本项目自身的代码（`src/`, `proj.win.cmake/` 等）采用 MIT 许可证。

本项目依赖于 Live2D Cubism SDK，请务必遵守其许可。