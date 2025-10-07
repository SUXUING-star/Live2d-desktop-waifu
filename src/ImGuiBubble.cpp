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

    _shouldShow = true;

    // --- 步骤一：定义样式和尺寸 ---
    const float maxWidth = 300.0f;           // 气泡最大宽度
    const ImVec2 padding(15.0f, 12.0f);      // 内部文字的边距
    const float rounding = 12.0f;            // 圆角大小
    const float border_thickness = 1.5f;     // 边框粗细
    const float tail_width = 20.0f;
    const float tail_height = 15.0f;
    const ImU32 bg_color = IM_COL32(255, 255, 255, 220); // 背景色: 白色半透明
    const ImU32 border_color = IM_COL32(0, 0, 0, 100);    // 边框色: 黑色半透明
    const ImU32 text_color = IM_COL32(0, 0, 0, 220);    // 字体颜色: 深灰色，比纯黑柔和

    // --- 步骤二：精确计算换行后文字需要的空间 ---
    // 关键: 计算宽度时要减去两边的 padding，这样文本区才不会挤到边框上
    ImVec2 textSize = ImGui::CalcTextSize(message.c_str(), NULL, false, maxWidth - padding.x * 2.0f);
    
    // --- 步骤三：计算整个气泡的最终大小 ---
    // 加上 padding，得到内容区的总大小
    ImVec2 content_size = ImVec2(textSize.x + padding.x * 2.0f, textSize.y + padding.y * 2.0f);
    
    // --- 步骤四：创建“无形”宿主窗口 ---
    // 锚点 (0.5, 1.0)，让气泡底部中心对准目标坐标
    ImGui::SetNextWindowPos(ImVec2(screenX, screenY), ImGuiCond_Always, ImVec2(0.5f, 1.0f));
    // 窗口尺寸要包含尖角的高度
    ImGui::SetNextWindowSize(ImVec2(content_size.x, content_size.y + tail_height));
    ImGui::SetNextWindowBgAlpha(0.0f); // 宿主窗口完全透明

    // 推入样式，取消宿主窗口自身的边框和内边距
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0,0));

    ImGui::Begin("BubbleHost", NULL,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse // 操，把滚动条和鼠标滚轮都给他禁了！
    );

    // --- 步骤五：画牛逼的带边框背景和无缝衔接的尖角 ---
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 p = ImGui::GetWindowPos(); // 宿主窗口左上角

    // 1. 先画边框 (一个比背景大一点的圆角矩形)
    draw_list->AddRectFilled(
        p,
        ImVec2(p.x + content_size.x, p.y + content_size.y),
        border_color,
        rounding
    );

    // 2. 再画背景 (在边框内层，小一点的圆角矩形)
    draw_list->AddRectFilled(
        ImVec2(p.x + border_thickness, p.y + border_thickness),
        ImVec2(p.x + content_size.x - border_thickness, p.y + content_size.y - border_thickness),
        bg_color,
        rounding > border_thickness ? rounding - border_thickness : 0.0f // 内层圆角要减去边框厚度
    );
    
    // 3. 画无缝衔接的尖角 (最骚的操作)
    //    尖角由两个三角形组成：一个大的做边框，一个小的做填充，完美覆盖接缝
    ImVec2 tail_p1 = ImVec2(p.x + content_size.x * 0.5f - tail_width * 0.5f, p.y + content_size.y - border_thickness);
    ImVec2 tail_p2 = ImVec2(tail_p1.x + tail_width, tail_p1.y);
    ImVec2 tail_p3 = ImVec2(tail_p1.x + tail_width * 0.5f, tail_p1.y + tail_height);

    // 先画大的黑色边框三角形
    draw_list->AddTriangleFilled(
        ImVec2(tail_p1.x - border_thickness, tail_p1.y),
        ImVec2(tail_p2.x + border_thickness, tail_p2.y),
        ImVec2(tail_p3.x, tail_p3.y + border_thickness),
        border_color
    );
    // 再画小的白色填充三角形，盖住边框三角形的顶部，实现无缝衔接
    draw_list->AddTriangleFilled(tail_p1, tail_p2, tail_p3, bg_color);


    // --- 步骤六：在画好的背景上，用 ImGui::TextWrapped 控件显示文字 ---
    // 它的好处是：原生支持换行，且不会产生滚动条！还能选中！
    ImGui::SetCursorPos(padding);
    ImGui::PushStyleColor(ImGuiCol_Text, text_color);
    ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + (maxWidth - padding.x * 2.0f)); // 设置换行宽度

    ImGui::TextUnformatted(message.c_str()); // 用 TextUnformatted 性能更好

    ImGui::PopTextWrapPos();
    ImGui::PopStyleColor();

    ImGui::End();

    ImGui::PopStyleVar(2);
}