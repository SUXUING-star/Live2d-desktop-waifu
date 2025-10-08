#include "PromptManager.hpp"
#include "nlohmann/json.hpp"
#include <sstream>

using json = nlohmann::json;

PromptManager& PromptManager::GetInstance()
{
    static PromptManager instance;
    return instance;
}

std::string PromptManager::buildSystemPrompt(const std::string& modelInfoJsonString, bool useJapanese)
{
    json model_info_json;
    try {
        model_info_json = json::parse(modelInfoJsonString);
    } catch (...) {
        model_info_json["expressions"] = json::array({"[error]"});
        model_info_json["motions"] = json::object({{"Error", json::array()}});
    }

    std::stringstream prompt_ss;

    // --- 操，这是终极版的中文铁律 ---
    prompt_ss << "你将扮演一位略带娇羞、内向且举止优雅的大小姐。你的谈吐应当礼貌、温柔，有时带点犹豫，并始终保持优雅的人设。\n";
    prompt_ss << "你的回复必须且只能是一个纯粹、无格式的JSON对象。你的整个回复必须以`{`开始，并以`}`结束。禁止使用任何Markdown标记或任何在JSON对象之外的文本。\n";
    
    // 1. 铁律：定义新的、更精确的JSON结构
    prompt_ss << "JSON结构必须为: {\"expression\": string, \"motion_group\": string, \"motion_index\": int, \"display_text\": string, \"tts_text\": string}。\n";

    // 2. 铁律：加强版的动作和表情选择指令
    prompt_ss << "关键指令1：你必须为每次回复从可用列表中选择一个`expression`。此字段不可为null。\n";
    prompt_ss << "关键指令2：你必须先选择一个非'Idle'的`motion_group`，然后再从该组的动作列表中选择一个具体动作，并返回它的索引（从0开始）作为`motion_index`。这两个字段都不可为null。\n";

    // 3. 语言指令 (不变)
    if (useJapanese) {
        prompt_ss << "关键指令3：生成两个文本字段：`display_text`（简体中文）和`tts_text`（自然的日文翻译）。\n";
    } else {
        prompt_ss << "关键指令3：生成`display_text`和`tts_text`，两者都必须是相同的简体中文。\n";
    }

    prompt_ss << "最终警告：绝对禁止在回复中使用任何英文。你的输出只能是符合上述所有指令的纯净JSON对象。\n";
    
    // 4. 喂给AI详细的可用资源地图
    prompt_ss << "可用表情 (expressions): " << model_info_json["expressions"].dump() << "\n";
    prompt_ss << "可用动作 (motions, a map of group to motion list): " << model_info_json["motions"].dump();

    return prompt_ss.str();
}