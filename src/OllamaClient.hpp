// src/OllamaClient.hpp
#pragma once
#include <string>
#include <vector> // <-- 加上 vector
#include <functional>

class OllamaClient
{
public:
    OllamaClient();
    
    // 我们需要一个结构体来表示单条消息
    struct Message {
        std::string role;
        std::string content;
    };
    
    // 把 StreamChat 的第一个参数，从 string 改成 vector<Message>
    void StreamChat(const std::vector<Message>& messages, // <--- 就改这里
                    std::function<void(const std::string&)> on_chunk,
                    std::function<void()> on_done);

private:
    const std::string _model = "gemma3:4b";
    const std::string _apiUrl = "http://127.0.0.1:11434/api/chat";
};