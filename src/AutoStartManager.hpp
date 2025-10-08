#pragma once
#include <string>

class AutoStartManager {
public:
    // 设置开机自启状态
    // enabled: true -> 添加自启, false -> 移除自启
    static bool SetAutoStart(bool enabled);

    // 检查当前是否已设置为开机自启
    static bool IsAutoStartEnabled();

private:
    // 获取当前可执行文件的完整路径
    static std::wstring GetExecutablePath();
};