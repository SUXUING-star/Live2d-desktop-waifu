// src/ChatWindow.hpp
#pragma once

#include "OllamaClient.hpp"
#include "TTSClient.hpp"
#include "imgui.h"
#include <vector>
#include <string>
#include <mutex>
#include <thread>
#include <atomic>

class ChatWindow
{
public:
    ChatWindow();
    void Render();
    void Open(ImVec2 position);

private:
    void SendMessage();

    // UI State
    bool _isOpen;
    ImVec2 _windowPos;
    char _inputBuffer[1024 * 4];

    // Data & Threading
    std::vector<OllamaClient::Message> _history;
    std::mutex _mutex; // 保护所有共享数据: _history, _streamingResponse, _isResponding

    // AI Response State
    std::atomic<bool> _isResponding;
    std::string _streamingResponse; // 用于UI实时显示打字机效果
    std::string _accumulatedJson;   // 用于最终完整解析

    // TTS Control
    bool _ttsEnabled = false;
    int _ttsLanguage = 1; // 0=中文, 1=日文
};