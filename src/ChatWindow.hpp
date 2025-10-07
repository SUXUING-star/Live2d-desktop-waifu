// src/ChatWindow.hpp
#pragma once

#include "OllamaClient.hpp" // 这个提供了 OllamaClient::Message
#include "TTSClient.hpp"
#include "imgui.h"
#include <vector>
#include <string>
#include <mutex>
#include <thread>

class ChatWindow
{
public:
    ChatWindow();
    
    void Render();
    void Open(ImVec2 position);

private:
    // 把你自己定义的那个 struct Message 删掉！就留下面这些！
    
    void SendMessage();

    bool _isOpen;
    ImVec2 _windowPos;
    OllamaClient _client;

    TTSClient _ttsClient;          // 语音客户端
    bool _ttsEnabled = false;      // 语音开关，默认关闭
    int _ttsLanguage = 1;          // 语言选择, 0=中文, 1=日文(默认)
    
    // _history 的类型是正确的，就用这个
    std::vector<OllamaClient::Message> _history;
    
    char _inputBuffer[1024 * 4];
    
    std::string _streamingResponse;
    std::string _accumulatedJson; 
    bool _isResponding;
    std::mutex _mutex;
};