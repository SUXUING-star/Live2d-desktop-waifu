#include "AutoStartManager.hpp"
#include <windows.h> // 操，Windows 核心 API
#include <vector>

// 我们给自启项起个独一无二的名字
const wchar_t* APP_NAME = L"SUXingLive2D"; 
// 注册表路径
const wchar_t* REG_PATH = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";

std::wstring AutoStartManager::GetExecutablePath() {
    std::vector<wchar_t> pathBuf;
    DWORD copied = 0;
    do {
        pathBuf.resize(pathBuf.size() + MAX_PATH);
        copied = GetModuleFileNameW(NULL, pathBuf.data(), pathBuf.size());
    } while (copied >= pathBuf.size());

    pathBuf.resize(copied);
    return std::wstring(pathBuf.begin(), pathBuf.end());
}

bool AutoStartManager::SetAutoStart(bool enabled) {
    HKEY hKey = NULL;
    // 打开注册表项，准备写入
    if (RegOpenKeyExW(HKEY_CURRENT_USER, REG_PATH, 0, KEY_WRITE, &hKey) != ERROR_SUCCESS) {
        return false;
    }

    bool success = false;
    if (enabled) {
        std::wstring exePath = GetExecutablePath();
        // 添加或修改键值对
        if (RegSetValueExW(hKey, APP_NAME, 0, REG_SZ, (const BYTE*)exePath.c_str(), (exePath.size() + 1) * sizeof(wchar_t)) == ERROR_SUCCESS) {
            success = true;
        }
    } else {
        // 删除键值对
        if (RegDeleteValueW(hKey, APP_NAME) == ERROR_SUCCESS) {
            success = true;
        }
    }

    RegCloseKey(hKey);
    return success;
}

bool AutoStartManager::IsAutoStartEnabled() {
    HKEY hKey = NULL;
    // 打开注册表项，准备读取
    if (RegOpenKeyExW(HKEY_CURRENT_USER, REG_PATH, 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
        return false;
    }

    // 尝试查询我们那个键值
    LSTATUS status = RegQueryValueExW(hKey, APP_NAME, NULL, NULL, NULL, NULL);
    RegCloseKey(hKey);
    
    // 如果能成功查询到 (ERROR_SUCCESS)，说明自启是开着的
    return status == ERROR_SUCCESS;
}