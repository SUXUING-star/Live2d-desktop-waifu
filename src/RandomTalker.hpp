#pragma once

#include <string>
#include <vector>
#include <mutex>
#include "OllamaClient.hpp"
#include "TTSClient.hpp"
#include <chrono> 

class RandomTalker
{
public:
    RandomTalker();

    // 核心触发函数
    void TriggerRandomTalk();

private:
    void SendToOllama(const std::string& prompt);
    void HandleOllamaResponse(const std::string& raw_json);
    std::mutex   _mutex;
     bool         _isBusy;
    std::chrono::steady_clock::time_point _lastTriggerTime;
    std::vector<std::string> _prompts;


};