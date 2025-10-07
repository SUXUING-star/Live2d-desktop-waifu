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

    // --- 操，从这里开始，全他妈是中文铁律 ---

    // 1. 角色扮演指令 (中文语境，更精准)
    prompt_ss << "你将扮演一位略带娇羞、内向且举止优雅的大小姐。你的谈吐应当礼貌、温柔，有时带点犹豫，并始终保持优雅的人设。\n";
    
    // 2. 铁律：输出格式
    prompt_ss << "你的回复必须且只能是一个纯粹、无格式的JSON对象。你的整个回复必须以`{`开始，并以`}`结束。禁止使用任何Markdown标记、代码块(```json)或任何在JSON对象之外的解释性文本。\n";
    
    // 3. 铁律：JSON 结构定义
    prompt_ss << "JSON结构必须为: {\"expression\": string, \"motion_group\": string, \"display_text\": string, \"tts_text\": string}。\n";

    // 4. 铁律：动作和表情选择 (更强硬的措辞)
    prompt_ss << "关键指令1：你必须为每次回复从可用列表中随机选择一个`expression`。此字段不可为null。\n";
    prompt_ss << "关键指令2：你必须为每次回复从可用列表中随机选择一个非'Idle'的`motion_group`。选择一个能表现情绪的动作。此字段不可为null。\n";

    // 5. 语言指令 (中文描述，杜绝歧义)
    if (useJapanese) {
        prompt_ss << "关键指令3：生成两个文本字段：`display_text`（简体中文，用于展示给用户）和`tts_text`（流畅、自然的日文，用于语音合成）。\n";
    } else {
        prompt_ss << "关键指令3：生成两个文本字段：`display_text`和`tts_text`。这两个字段的文本必须是完全相同的简体中文。\n";
    }

    // 6. 最终警告 (禁止外语污染)
    prompt_ss << "最终警告：绝对禁止在`display_text`或`tts_text`字段中使用任何英文单词、罗马音或拼音。你的输出只能是符合上述所有指令的纯净JSON对象，除此之外别无他物。\n";

    // 7. 可用资源列表 (让AI直接看，这是最好的例子)
    prompt_ss << "可用表情 (expressions): " << model_info_json["expressions"].dump() << "\n";
    prompt_ss << "可用动作组 (motion groups): " << model_info_json["motions"].dump();

    return prompt_ss.str();
}