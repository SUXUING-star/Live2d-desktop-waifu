#include "ImGuiBubble.hpp"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "LAppDefine.hpp" // For ResourcesPath
#include <GLFW/glfw3.h> // For glfwGetFramebufferSize

// --- 静态成员，负责全局的ImGui初始化和关闭 ---

void ImGuiBubble::Initialize(void* windowHandle)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    // ======== 操！加载中文字体！========
    std::string font_path = std::string(LAppDefine::ResourcesPath) + "font.otf";
    io.Fonts->AddFontFromFileTTF(font_path.c_str(), 16.0f, NULL, io.Fonts->GetGlyphRangesChineseFull());
    
    // ======== 操！在这里设置你的白色半透明主题！========
    ImGuiStyle& style = ImGui::GetStyle();
    
    // --- 白底半透明主题 ---
    // 我把所有背景色的 Alpha (第四个参数) 都调低了
    float bg_alpha = 0.85f; // 你可以调整这个值，比如 0.7, 0.9, 随意

    style.Colors[ImGuiCol_Text]          = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
    style.Colors[ImGuiCol_WindowBg]      = ImVec4(1.00f, 1.00f, 1.00f, bg_alpha);
    style.Colors[ImGuiCol_ChildBg]       = ImVec4(0.95f, 0.95f, 0.95f, bg_alpha);
    style.Colors[ImGuiCol_PopupBg]       = ImVec4(1.00f, 1.00f, 1.00f, 0.95f);
    style.Colors[ImGuiCol_Border]        = ImVec4(0.50f, 0.50f, 0.50f, 0.50f);
    style.Colors[ImGuiCol_FrameBg]       = ImVec4(0.88f, 0.88f, 0.88f, 0.54f);
    style.Colors[ImGuiCol_FrameBgHovered]= ImVec4(0.95f, 0.95f, 0.95f, 0.40f);
    style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.95f, 0.95f, 0.95f, 0.67f);
    style.Colors[ImGuiCol_TitleBg]       = ImVec4(0.85f, 0.85f, 0.85f, 1.00f);
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.20f, 0.60f, 0.80f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarBg]   = ImVec4(0.98f, 0.98f, 0.98f, 0.53f);
    style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.69f, 0.69f, 0.69f, 0.80f);
    style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.49f, 0.49f, 0.49f, 0.80f);
    style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.49f, 0.49f, 0.49f, 1.00f);
    style.Colors[ImGuiCol_CheckMark]     = ImVec4(0.20f, 0.60f, 0.80f, 1.00f);
    style.Colors[ImGuiCol_SliderGrab]    = ImVec4(0.20f, 0.60f, 0.80f, 0.78f);
    style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.46f, 0.54f, 0.80f, 0.60f);
    style.Colors[ImGuiCol_Button]        = ImVec4(0.88f, 0.88f, 0.88f, 0.40f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.95f, 0.95f, 0.95f, 1.00f);
    style.Colors[ImGuiCol_ButtonActive]  = ImVec4(0.75f, 0.75f, 0.75f, 1.00f);
    style.Colors[ImGuiCol_Header]        = ImVec4(0.20f, 0.60f, 0.80f, 0.31f);
    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.20f, 0.60f, 0.80f, 0.80f);
    style.Colors[ImGuiCol_HeaderActive]  = ImVec4(0.20f, 0.60f, 0.80f, 1.00f);
    style.Colors[ImGuiCol_Separator]     = ImVec4(0.39f, 0.39f, 0.39f, 0.62f);
    style.Colors[ImGuiCol_ResizeGrip]    = ImVec4(1.00f, 1.00f, 1.00f, 0.0f);

    style.WindowPadding    = ImVec2(8, 8);
    style.FramePadding     = ImVec2(5, 4);
    style.ItemSpacing      = ImVec2(8, 4);
    style.WindowRounding   = 5.0f;
    style.FrameRounding    = 4.0f;
    style.ScrollbarSize    = 14.0f;
    style.ScrollbarRounding= 9.0f;
    style.GrabMinSize      = 10.0f;
    style.GrabRounding     = 3.0f;
    style.WindowBorderSize = 1.0f;
    style.FrameBorderSize  = 1.0f;
    style.ChildBorderSize  = 1.0f;
    
    // --- 初始化后端 ---
    ImGui_ImplGlfw_InitForOpenGL(static_cast<GLFWwindow*>(windowHandle), true);
    ImGui_ImplOpenGL3_Init("#version 130");
}


void ImGuiBubble::Shutdown()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void ImGuiBubble::BeginFrame()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void ImGuiBubble::RenderFrame()
{
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}


// --- 成员函数，负责单个气泡的显示 ---

ImGuiBubble::ImGuiBubble() : _screenX(0), _screenY(0), _shouldShow(false)
{
}

ImGuiBubble::~ImGuiBubble()
{
}

void ImGuiBubble::Show(float screenX, float screenY, const std::string& message)
{
    if (message.empty()) 
    {
        _shouldShow = false;
        return;
    }

    _screenX = screenX;
    _screenY = screenY;
    _message = message;
    _shouldShow = true;

    if (_shouldShow)
    {
        // --- 步骤一：计算文字需要多大的空间 ---
         ImGui::PushFont(ImGui::GetFont());
        ImVec2 textSize = ImGui::CalcTextSize(_message.c_str(), NULL, false, 300.0f);
        ImGui::PopFont();
        
        // --- 步骤二：定义样式和尺寸 ---
        ImVec2 padding(15, 15);

        float rounding = 12.0f; // 你可以改成 8.0f, 10.0f, 15.0f... 随便你喜欢

        float tail_width = 20.0f;
        float tail_height = 15.0f;
        ImU32 bg_color = IM_COL32(255, 255, 255, 220);
        ImU32 text_color = IM_COL32(0, 0, 0, 255);

        // --- 步骤三：计算整个气泡（包括尖角）的最终位置和大小 ---
        ImVec2 bubble_pos = ImVec2(_screenX - (textSize.x/2.0f) - padding.x, _screenY - textSize.y - padding.y*2.0f - tail_height);
        ImVec2 bubble_size = ImVec2(textSize.x + padding.x * 2.0f, textSize.y + padding.y * 2.0f);
        
        // 我们用一个“无形”的窗口来容纳我们的自定义绘制
        ImGui::SetNextWindowPos(ImVec2(_screenX, _screenY), ImGuiCond_Always, ImVec2(0.0f, 1.0f));
        ImGui::SetNextWindowSize(ImVec2(bubble_size.x, bubble_size.y + tail_height));
        ImGui::SetNextWindowBgAlpha(0.0f); // 关键！让ImGui的默认背景完全透明！

        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

        ImGui::Begin("BubbleHost", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing);
        
        // --- 步骤四：自己画背景和尖角！---
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        ImVec2 p = ImGui::GetWindowPos();
        
        // 1. 画主体（一个圆角拉满的矩形，就是椭圆）
        draw_list->AddRectFilled(
            p, 
            ImVec2(p.x + bubble_size.x, p.y + bubble_size.y), 
            bg_color, 
            rounding
        );

        // 2. 画尖角 (这次它在我们的“无形”窗口之内，绝对不会被裁掉)
        ImVec2 p1 = ImVec2(p.x + bubble_size.x * 0.2f, p.y + bubble_size.y);
        ImVec2 p2 = ImVec2(p1.x + tail_width, p1.y);
        ImVec2 p3 = ImVec2(p1.x + tail_width * 0.5f, p1.y + tail_height);
        draw_list->AddTriangleFilled(p1, p2, p3, bg_color);
        
        // --- 步骤五：在画好的背景上写字 ---
        draw_list->AddText(
            ImVec2(p.x + padding.x, p.y + padding.y),
            text_color,
            _message.c_str()
        );

        ImGui::End();

        ImGui::PopStyleVar();
    }
}