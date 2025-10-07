#include "RandomTalker.hpp"
#include "LAppDelegate.hpp"
#include "LAppLive2DManager.hpp"
#include "LAppModel.hpp"
#include "ICubismModelSetting.hpp"
#include "nlohmann/json.hpp"
#include <iostream>
#include <chrono>
#include "PromptManager.hpp"
#include <random>           // 为了用 std::mt19937 这些随机数工具
#include "LAppDefine.hpp"    // 为了用 LAppDefine::PriorityForce

using json = nlohmann::json;

RandomTalker::RandomTalker() : _isBusy(false), _lastTriggerTime(std::chrono::steady_clock::now())
{
    _prompts = {
        "和我聊聊天气吧", "讲个冷笑话", "今天有什么趣事吗？",
        "你喜欢什么？", "随便说点什么吧", "你现在在想什么？",
        "今天心情怎么样？", "推荐一首歌给你"
    };
}

void RandomTalker::TriggerRandomTalk()
{
    auto now = std::chrono::steady_clock::now();
    if (std::chrono::duration_cast<std::chrono::seconds>(now - _lastTriggerTime).count() < 2)
    {
        return;
    }

    std::unique_lock<std::mutex> lock(_mutex);
    if (_isBusy)
    {
        return;
    }
    lock.unlock();
    
    _lastTriggerTime = now;

    if (_prompts.empty()) return;

    std::random_device rd;
    // --- 操，这里是错别字，mt19377 改成 mt19937 ---
    std::mt19937 gen(rd()); 
    std::uniform_int_distribution<> distrib(0, _prompts.size() - 1);
    const std::string& random_prompt = _prompts[distrib(gen)];

    std::thread([this, random_prompt]() {
        this->SendToOllama(random_prompt);
    }).detach();
}

void RandomTalker::SendToOllama(const std::string& prompt)
{
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _isBusy = true;
    }

    // 1. 获取模型可用资源信息
    std::string model_info_string = LAppDelegate::GetInstance()->GetModelInfoAsJsonString();
    
    // --- 操，核心修改在这里 ---
    // 2. 调用单例的 PromptManager，并强制使用日文 (true)
    std::string system_prompt = PromptManager::GetInstance().buildSystemPrompt(model_info_string, true);
    // --- 修改结束 ---

    // 3. 准备要发送给 Ollama 的消息 (随机对话不需要历史记录)
    std::vector<OllamaClient::Message> messages_to_send = {
        {"system", system_prompt},
        {"user", prompt}
    };
    
    // 4. 设置流式回调函数
    std::string accumulated_json;
    auto on_chunk = [&](const std::string& content_chunk) {
        accumulated_json += content_chunk;
    };
    
    auto on_done = [&]() {
        // HandleOllamaResponse 是我们之前写好的处理函数，这里直接调用
        HandleOllamaResponse(accumulated_json);
        
        // 请求结束后，解锁繁忙状态
        std::lock_guard<std::mutex> lock(_mutex);
        _isBusy = false;
    };
    
    // 5. 执行请求
    _ollamaClient.StreamChat(messages_to_send, on_chunk, on_done);
}


void RandomTalker::HandleOllamaResponse(const std::string& raw_json)
{
    try
    {
        size_t first_brace = raw_json.find('{');
        size_t last_brace = raw_json.rfind('}');
        if (first_brace == std::string::npos || last_brace == std::string::npos) {
             throw std::runtime_error("No JSON object found in response");
        }
        std::string clean_json_str = raw_json.substr(first_brace, last_brace - first_brace + 1);
        json final_json = json::parse(clean_json_str);

        LAppModel* model = LAppLive2DManager::GetInstance()->GetModel(0);
        if (model) {
            if (final_json.contains("expression") && final_json["expression"].is_string()) {
                model->SetExpression(final_json["expression"].get<std::string>().c_str());
            }

            if (final_json.contains("motion_group") && final_json["motion_group"].is_string()) {
                std::string group = final_json["motion_group"];
                if (model->GetModelSetting()->GetMotionCount(group.c_str()) > 0) {
                    model->StartRandomMotion(group.c_str(), LAppDefine::PriorityForce);
                }
            }
        }
        
        std::string display_text = final_json.value("display_text", "[内容错误]");
        std::string tts_text = final_json.value("tts_text", "");

        float bubble_duration = tts_text.empty() ? 5.0f : (_ttsClient.EstimateDuration(tts_text) + 1.0f);
        LAppDelegate::GetInstance()->ShowBubbleMessage(display_text, bubble_duration); 

        if (!tts_text.empty()) {
            _ttsClient.Speak(tts_text, "日文");
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "[RandomTalker] Error handling response: " << e.what() << "\nRaw: " << raw_json << std::endl;
        LAppDelegate::GetInstance()->ShowBubbleMessage("[处理回复时出错]", 3.0f);
    }
}