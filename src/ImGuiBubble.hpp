#pragma once
#include <string>

// 一个用Dear ImGui实现的、可复用、可定制样式的牛逼气泡类
class ImGuiBubble
{
public:
    ImGuiBubble();
    ~ImGuiBubble();

    // 初始化 ImGui 上下文和样式
    static void Initialize(void* windowHandle);
    
    // 关闭 ImGui
    static void Shutdown();

    // 在主循环的开始调用，开始新的一帧
    static void BeginFrame();
    
    // 在主循环的结束调用，渲染所有UI
    static void RenderFrame();

    // 显示一个气泡消息
    void Show(float screenX, float screenY, const std::string& message);

private:
    std::string _message;
    float _screenX, _screenY;
    bool _shouldShow;
};