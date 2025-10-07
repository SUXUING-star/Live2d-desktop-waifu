#pragma once
#include <string>

// 操，用单例模式，全局就一个实例，方便调用
class PromptManager
{
public:
    // 获取单例实例的唯一方法
    static PromptManager& GetInstance();

    // 构建牛逼的系统提示词的核心函数
    std::string buildSystemPrompt(const std::string& modelInfoJsonString, bool useJapanese);

private:
    // 私有构造函数，防止外部创建实例
    PromptManager() = default; 
    
    // 删除拷贝构造和赋值操作，保证单例的纯洁性
    PromptManager(const PromptManager&) = delete;
    PromptManager& operator=(const PromptManager&) = delete;
};