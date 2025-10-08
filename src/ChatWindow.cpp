

// src/ChatWindow.cpp


#define NOMINMAX
#include <algorithm>

#include "ChatWindow.hpp"
#include "LAppDelegate.hpp"
#include "LAppLive2DManager.hpp"
#include "LAppModel.hpp"
#include "LAppDefine.hpp"              
#include "ICubismModelSetting.hpp"
#include "imgui.h"
#include "imgui_internal.h"
#include "nlohmann/json.hpp"
#include <iostream> 
#include "PromptManager.hpp"

using json = nlohmann::json;

ChatWindow::ChatWindow() : _isOpen(false), _isResponding(false), _accumulatedJson("")
{
    memset(_inputBuffer, 0, sizeof(_inputBuffer));
    _windowPos = ImVec2(0, 0);
}

void ChatWindow::Open(ImVec2 position)
{
    _isOpen = true;
    _windowPos = position;
}

void ChatWindow::Render()
{
    if (!_isOpen) return;

    ImGuiStyle& style = ImGui::GetStyle();
    ImGui::SetNextWindowPos(_windowPos, ImGuiCond_Appearing);
    ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);

    if (!ImGui::Begin("与我对话", &_isOpen))
    {
        ImGui::End();
        if (!_isOpen) {
            _history.clear(); _streamingResponse.clear(); _isResponding = false;
            LAppDelegate::GetInstance()->GetTTSClient()->Stop();
            memset(_inputBuffer, 0, sizeof(_inputBuffer));
        }
        return;
    }

    // --- 聊天记录显示区域 ---
    const float footer_height_to_reserve = style.ItemSpacing.y * 2 + (ImGui::GetTextLineHeightWithSpacing() * 3) + 45;
    if (ImGui::BeginChild("ScrollingRegion", ImVec2(0, -footer_height_to_reserve), true, ImGuiWindowFlags_None))
    {
        std::lock_guard<std::mutex> lock(_mutex);
        
        for (const auto& msg : _history)
        {
            ImGui::PushID(&msg);

            // 用一个 Group 把标签和文本框包起来
            ImGui::BeginGroup(); 
            {
                if (msg.role == "user") ImGui::TextColored(ImVec4(0.1f, 0.4f, 0.1f, 1.0f), "我:");
                else ImGui::TextColored(ImVec4(0.2f, 0.2f, 0.7f, 1.0f), "她:");
                
                ImGui::SameLine();

                // ======== 操！这次我们用终极版的 InputTextMultiline！========
                
                ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0,0,0,0));
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0,0));
                ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);

                const ImGuiInputTextFlags flags = ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_NoHorizontalScroll;
                
                // 我们不再手动计算高度！让 InputTextMultiline 自己决定！
                // ImVec2(0,0) 会让它自动填充剩余空间，但是这样会导致所有消息都一样高。
                // 我们需要一种方法让它“自适应”。
                // 最可靠的方法是：先用 TextWrapped 计算一次高度，再把这个高度给 InputTextMultiline
                
                float wrap_width = ImGui::GetContentRegionAvail().x - ImGui::GetCursorPosX();
                ImVec2 text_size = ImGui::CalcTextSize(msg.content.c_str(), NULL, false, wrap_width);
                
                char* buffer = const_cast<char*>(msg.content.c_str());
                
                // 给它一个精确的高度，再加上一点点 buffer
                float required_height = text_size.y + style.ItemSpacing.y;
                ImGui::InputTextMultiline("##text", buffer, msg.content.size() + 1, ImVec2(wrap_width, required_height), flags);
                
                ImGui::PopStyleVar(2);
                ImGui::PopStyleColor();
            }
            ImGui::EndGroup();

            ImGui::PopID();
        }

        if (_isResponding && _history.empty()) ImGui::Text("她: 思考中...");
        if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 10.0f) ImGui::SetScrollHereY(1.0f);
    }
    ImGui::EndChild();

    ImGui::Separator();

    // --- 输入和发送区域 (保持我上次给你写的、带按钮的版本) ---
    bool reclaim_focus = false;
    bool should_send = false;
    if (!_isResponding) {
        float button_width = ImGui::CalcTextSize("发送").x + style.FramePadding.x * 2.0f;
        ImGui::PushItemWidth(-(button_width + style.ItemSpacing.x));
        ImGui::InputTextMultiline("##Input", _inputBuffer, sizeof(_inputBuffer), ImVec2(0, ImGui::GetTextLineHeightWithSpacing() * 3));
        ImGui::PopItemWidth();
        if (ImGui::IsItemFocused() && ImGui::IsKeyPressed(ImGuiKey_Enter, false) && !ImGui::GetIO().KeyShift) {
            should_send = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("发送", ImVec2(button_width, ImGui::GetTextLineHeightWithSpacing() * 3))) {
            should_send = true;
        }
        if (should_send) {
            std::string text_to_send = _inputBuffer;
            while (!text_to_send.empty() && std::isspace(static_cast<unsigned char>(text_to_send.back()))) { text_to_send.pop_back(); }
            if (!text_to_send.empty()) {
                SendMessage();
                reclaim_focus = true;
                memset(_inputBuffer, 0, sizeof(_inputBuffer));
            } else {
                memset(_inputBuffer, 0, sizeof(_inputBuffer));
            }
        }
    } else {
        ImGui::TextDisabled("AI 正在输入...");
        ImGui::Dummy(ImVec2(-1.0f, ImGui::GetTextLineHeightWithSpacing() * 3 + style.FramePadding.y * 2 + style.ItemSpacing.y));
    }

    // --- 语音控制区域 ---
    if (ImGui::Checkbox("开启语音", &_ttsEnabled)) { if (!_ttsEnabled) LAppDelegate::GetInstance()->GetTTSClient()->Stop(); }
    ImGui::SameLine(); ImGui::Spacing(); ImGui::SameLine();
    ImGui::BeginDisabled(!_ttsEnabled);
    ImGui::RadioButton("中文", &_ttsLanguage, 0); ImGui::SameLine(); ImGui::RadioButton("日文", &_ttsLanguage, 1);
    ImGui::EndDisabled();

    if (reclaim_focus) { ImGui::SetKeyboardFocusHere(-1); }
    
    ImGui::End();

    if (!_isOpen) {
        _history.clear(); _streamingResponse.clear(); _isResponding = false;
        LAppDelegate::GetInstance()->GetTTSClient()->Stop();
        memset(_inputBuffer, 0, sizeof(_inputBuffer));
    }
}
void ChatWindow::SendMessage()
{
    std::string user_prompt = _inputBuffer;
    
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _history.push_back({"user", user_prompt});
        _isResponding = true;
        _accumulatedJson = "";
    }
    
    memset(_inputBuffer, 0, sizeof(_inputBuffer));

    std::thread([this]() {
        
        // 1. 获取模型可用资源信息
        std::string model_info_string = LAppDelegate::GetInstance()->GetModelInfoAsJsonString();
        
        // --- 操，核心修改在这里 ---
        // 2. 根据UI选项决定是否使用日文TTS
        bool use_japanese = (_ttsEnabled && _ttsLanguage == 1);
        
        // 3. 调用单例的 PromptManager 生成终极系统提示词
        std::string system_prompt = PromptManager::GetInstance().buildSystemPrompt(model_info_string, use_japanese);
        // --- 修改结束 ---

        // 4. 准备要发送给 Ollama 的消息历史
        std::vector<OllamaClient::Message> messages_to_send;
        messages_to_send.push_back({"system", system_prompt});
        {
            std::lock_guard<std::mutex> lock(_mutex);
            const int history_limit = 5; 
            int start = std::max(0, (int)_history.size() - history_limit);
            for(size_t i = start; i < _history.size(); ++i) {
                 messages_to_send.push_back(_history[i]);
            }
        }
        
        // 5. 设置流式回调函数
        auto on_chunk = [this](const std::string& content_chunk) {
            std::lock_guard<std::mutex> lock(_mutex);
            _accumulatedJson += content_chunk;
        };
        
        auto on_done = [this]() {
            std::lock_guard<std::mutex> lock(_mutex);
            
            try
            {
                std::string raw_response = _accumulatedJson;
                std::string clean_json_str;

                size_t first_brace = raw_response.find('{');
                size_t last_brace = raw_response.rfind('}');
                
                if (first_brace != std::string::npos && last_brace != std::string::npos && last_brace > first_brace) {
                    clean_json_str = raw_response.substr(first_brace, last_brace - first_brace + 1);
                }
                
                json final_json = json::parse(clean_json_str);
                
                LAppModel* model = LAppLive2DManager::GetInstance()->GetModel(0);

                 if (model) {
                    // 表情逻辑不变
                    if (final_json.contains("expression") && final_json["expression"].is_string()) {
                        model->SetExpression(final_json["expression"].get<std::string>().c_str());
                    }

                    // --- 操，核心执行逻辑修改在这里 ---
                    if (final_json.contains("motion_group") && final_json["motion_group"].is_string() &&
                        final_json.contains("motion_index") && final_json["motion_index"].is_number_integer())
                    {
                        std::string group = final_json["motion_group"].get<std::string>();
                        int index = final_json["motion_index"].get<int>();

                        // 安全检查：确保group存在，且index在范围内
                        if (model->GetModelSetting()->GetMotionCount(group.c_str()) > index && index >= 0) {
                            // 调用精确播放函数！
                            model->StartMotion(group.c_str(), index, LAppDefine::PriorityForce);
                        }
                    }
                }
                if (final_json.contains("display_text") && final_json["display_text"].is_string()) {
                    std::string display_text = final_json["display_text"].get<std::string>();
                    if (!display_text.empty()) {
                        _history.push_back({"assistant", display_text});
                    }
                    if (_ttsEnabled && final_json.contains("tts_text") && final_json["tts_text"].is_string()) {
                        std::string tts_text = final_json["tts_text"].get<std::string>();
                        std::string lang_for_tts = (_ttsLanguage == 0) ? "中文" : "日文";
                        LAppDelegate::GetInstance()->GetTTSClient()->Speak(tts_text, lang_for_tts);
                    }
                }
            }
            catch (json::parse_error& e)
            {
                 _history.push_back({"assistant", "[JSON Parse Error] " + std::string(e.what()) + "\nRaw: " + _accumulatedJson});
            }

            _streamingResponse.clear(); 
            _accumulatedJson.clear();
            _isResponding = false;
        };
        
        // 6. 执行请求
        LAppDelegate::GetInstance()->GetOllamaClient()->StreamChat(messages_to_send, on_chunk, on_done);

    }).detach();
}