/**
 * Copyright(c) Live2D Inc. All rights reserved.
 *
 * Use of this source code is governed by the Live2D Open Software license
 * that can be found at https://www.live2d.com/eula/live2d-open-software-license-agreement_en.html.
 */

#include "LAppDelegate.hpp"
#include <iostream>
#include <GL/glew.h>

#if defined(_WIN32)
    #define GLFW_EXPOSE_NATIVE_WIN32
#endif
// =======================================================

#include <GLFW/glfw3.h> // GLFW 主头文件紧随其后


#if defined(_WIN32)
    #include <GLFW/glfw3native.h>
    #include <windows.h>
#endif
// =======================================================
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "LAppView.hpp"
#include "LAppPal.hpp"
#include "LAppDefine.hpp"
#include "LAppLive2DManager.hpp"
#include "LAppTextureManager.hpp"

#include "PlatformUtils.hpp"
#include "LAppModel.hpp"                 
#include "ICubismModelSetting.hpp"
#include "nlohmann/json.hpp" 


using namespace Csm;
using namespace std;
using namespace LAppDefine;

namespace {
    LAppDelegate* s_instance = NULL;

    // --- 新增键盘回调函数 ---
    void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
    {
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        {
            LAppDelegate::GetInstance()->AppEnd();
        }
    }
    // --- 新增滚轮回调函数 ---
    void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
    {
        if (ImGui::GetIO().WantCaptureMouse)
        {
            return;
        }
        // yoffset > 0 是向上滚（放大），yoffset < 0 是向下滚（缩小）
        if (yoffset > 0)
        {
            LAppDelegate::GetInstance()->GetView()->GetViewMatrix()->ScaleRelative(1.1f, 1.1f);
        }
        else if (yoffset < 0)
        {
            LAppDelegate::GetInstance()->GetView()->GetViewMatrix()->ScaleRelative(0.9f, 0.9f);
        }
    }
}

LAppDelegate* LAppDelegate::GetInstance()
{
    if (s_instance == NULL)
    {
        s_instance = new LAppDelegate();
    }

    return s_instance;
}

void LAppDelegate::ReleaseInstance()
{
    if (s_instance != NULL)
    {
        delete s_instance;
    }

    s_instance = NULL;
}

bool LAppDelegate::Initialize()
{
    if (DebugLogEnable)
    {
        LAppPal::PrintLogLn("START");
    }

    // GLFWの初期化
    if (glfwInit() == GL_FALSE)
    {
        if (DebugLogEnable)
        {
            LAppPal::PrintLogLn("Can't initilize GLFW");
        }
        return GL_FALSE;
    }

    #if defined(_WIN32)
    // 开启无边框
    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
    // 开启帧缓冲透明
    glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);
    // 窗口总在最前
    // glfwWindowHint(GLFW_FLOATING, GLFW_TRUE);
    #endif

    // Windowの生成_
    // _window = glfwCreateWindow(RenderTargetWidth, RenderTargetHeight, "SAMPLE", NULL, NULL);

       // 1. 获取主显示器的信息
    const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    if (!mode)
    {
        if (DebugLogEnable)
        {
            LAppPal::PrintLogLn("Failed to get video mode for primary monitor.");
        }
        glfwTerminate();
        return GL_FALSE;
    }

    // 2. 创建一个比屏幕高度小 1 个像素的窗口，来骗过“全屏独占”检测
    _window = glfwCreateWindow(mode->width, mode->height - 1, "Live2D Model", NULL, NULL);

    // 3. (可选但建议) 创建窗口后，立刻把它定位到屏幕的左上角 (0, 0)
    glfwSetWindowPos(_window, 0, 0);

    if (_window == NULL)
    {
        if (DebugLogEnable)
        {
            LAppPal::PrintLogLn("Can't create GLFW window.");
        }
        glfwTerminate();
        return GL_FALSE;
    }

    // Windowのコンテキストをカレントに設定
    glfwMakeContextCurrent(_window);
    glfwSwapInterval(1);

    if (glewInit() != GLEW_OK) {
        if (DebugLogEnable)
        {
            LAppPal::PrintLogLn("Can't initilize glew.");
        }
        glfwTerminate();
        return GL_FALSE;
    }

    //テクスチャサンプリング設定
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    //透過設定
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    //コールバック関数の登録
    glfwSetMouseButtonCallback(_window, EventHandler::OnMouseCallBack);
    glfwSetCursorPosCallback(_window, EventHandler::OnMouseCallBack);

    
    glfwSetKeyCallback(_window, KeyCallback);
    glfwSetScrollCallback(_window, ScrollCallback);

    // ウィンドウサイズ記憶
    int width, height;
    glfwGetWindowSize(LAppDelegate::GetInstance()->GetWindow(), &width, &height);
    _windowWidth = width;
    _windowHeight = height;

    // Cubism SDK の初期化
    InitializeCubism();

    //AppViewの初期化
    _view->Initialize(width, height);

    ImGuiBubble::Initialize(_window);

    BuildMenuItems(); 

    return GL_TRUE;
}

void LAppDelegate::Release()
{

    ImGuiBubble::Shutdown();

    // Windowの削除
    glfwDestroyWindow(_window);

    glfwTerminate();

    delete _textureManager;
    delete _view;

    // リソースを解放
    LAppLive2DManager::ReleaseInstance();

    //Cubism SDK の解放
    CubismFramework::Dispose();
}

void LAppDelegate::Run()
{
    //メインループ
    while (glfwWindowShouldClose(_window) == GL_FALSE && !_isEnd)
    {

        glfwPollEvents();


        ImGuiBubble::BeginFrame(); // 开始UI帧
        
        LAppPal::UpdateTime();     // 更新你的应用时间

        int width, height;
        glfwGetWindowSize(LAppDelegate::GetInstance()->GetWindow(), &width, &height);
        if( (_windowWidth!=width || _windowHeight!=height) && width>0 && height>0)
        {
            //AppViewの初期化
            _view->Initialize(width, height);
            // スプライトサイズを再設定
            _view->ResizeSprite();
            // サイズを保存しておく
            _windowWidth = width;
            _windowHeight = height;

            // ビューポート変更
            glViewport(0, 0, width, height);
        }

        
    
        
        // 先获取 ImGui 的 IO 对象，它包含了所有的输入状态
        ImGuiIO& io = ImGui::GetIO();
        
        // 1. 判断是否应该处理我们自己的逻辑 (鼠标没被ImGui的UI占用)
        if (!io.WantCaptureMouse)
        {
            // 2. 处理右键 -> 打开菜单
            // IsMouseClicked(1) 表示右键是否在本帧被“按下”
            if (ImGui::IsMouseClicked(1)) 
            {
                _contextMenu.Open();
            }

            // 3. 处理左键 -> 拖拽模型
            // IsMouseDown(0) 表示左键是否“持续按住”
            if (ImGui::IsMouseDown(0))
            {
                if (!_captured) // 如果是刚开始按
                {
                    _captured = true;
                    _view->OnTouchesBegan(_mouseX, _mouseY);
                }
            }
            else // 如果左键没按住
            {
                if (_captured) // 如果上一帧是按住的，说明是“松开”了
                {
                    _captured = false;
                    _view->OnTouchesEnded(_mouseX, _mouseY);
                }
            }
        }

        // 画面の初期化
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f); // RGBA，最后一个A改成0.0
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glClearDepth(1.0);

        //描画更新
        _view->Render();


        // 获取当前时间
        double currentTime = glfwGetTime();
        
        // 如果需要显示气泡
        if (currentTime < _bubbleExpireTime)
        {
            
            // 1. 在模型本地坐标系里，计算出最终的锚点位置
            float finalModelX = _bubbleModelAnchorX + _bubbleModelOffsetX;
            float finalModelY = _bubbleModelAnchorY + _bubbleModelOffsetY;

            // 2. 把这个最终计算好的模型坐标，一次性转换到屏幕坐标      
            float bubbleScreenX, bubbleScreenY;
            _view->ProjectToScreen(finalModelX, finalModelY, bubbleScreenX, bubbleScreenY);
        
            // 调用气泡对象的Show方法
            _bubble.Show(bubbleScreenX, bubbleScreenY, _bubbleMessage);
        }
        else 
        {
            // 时间到了，确保消息是空的，这样下次就不会显示了
            _bubbleMessage.clear();
        }

        _contextMenu.Render();
        _chatWindow.Render();

        if (_isSearchBoxOpen)
        {
            // --- 加这行，告诉ImGui窗口下次出现的位置 ---
            ImGui::SetNextWindowPos(_searchBoxPos); 
            // 让 ImGui 下一帧自动聚焦到这个窗口上
            ImGui::SetNextWindowFocus();
            // 设置窗口初始大小
            ImGui::SetNextWindowSize(ImVec2(300, 0));

            // 开始画一个叫“快速搜索”的窗口
            if (ImGui::Begin("快速搜索", &_isSearchBoxOpen))
            {
                // `ImGuiInputTextFlags_EnterReturnsTrue` 这个标志位能让用户按回车时，函数返回 true
                if (ImGui::InputText("##search_input", _searchBuffer, sizeof(_searchBuffer), ImGuiInputTextFlags_EnterReturnsTrue))
                {
                    // 如果用户按了回车，而且输入框里有东西
                    if (strlen(_searchBuffer) > 0)
                    {
                        PlatformUtils::OpenUrlWithQuery(_searchBuffer);
                        _isSearchBoxOpen = false;       // 搜完就把窗口关了
                    }
                }
                
                ImGui::SameLine(); // 让下一个控件跟在输入框后面

                if (ImGui::Button("搜索"))
                {
                     // 如果用户点了按钮，而且输入框里有东西
                    if (strlen(_searchBuffer) > 0)
                    {
                        PlatformUtils::OpenUrlWithQuery(_searchBuffer);  // 执行搜索！
                        _isSearchBoxOpen = false;       // 搜完就把窗口关了
                    }
                }
            }
            ImGui::End();

            // 如果用户点了窗口右上角的'X'关掉了窗口，_isSearchBoxOpen 会被 ImGui 自动设为 false
        }

        ImGuiBubble::RenderFrame(); // 渲染所有UI


        #if defined(_WIN32)
            HWND hwnd = glfwGetWin32Window(_window);
            
            // 获取当前鼠标在窗口内的坐标
            double xpos, ypos;
            glfwGetCursorPos(_window, &xpos, &ypos);
            int winWidth, winHeight;
            glfwGetWindowSize(_window, &winWidth, &winHeight);

            // 读取鼠标下方的像素alpha值
            unsigned char alpha = 0;
            if (xpos >= 0 && xpos < winWidth && ypos >= 0 && ypos < winHeight)
            {
                // 注意：OpenGL的坐标原点在左下角，而窗口坐标原点在左上角
                glReadPixels(static_cast<int>(xpos), winHeight - 1 - static_cast<int>(ypos), 1, 1, GL_ALPHA, GL_UNSIGNED_BYTE, &alpha);
            }

            // 根据alpha值决定是否穿透
            LONG_PTR style = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
            if (alpha == 0)
            {
                // 透明区域，设置穿透
                SetWindowLongPtr(hwnd, GWL_EXSTYLE, style | WS_EX_LAYERED | WS_EX_TRANSPARENT);
            }
            else
            {
                // 非透明区域（模型上），取消穿透，以便响应点击
                SetWindowLongPtr(hwnd, GWL_EXSTYLE, (style | WS_EX_LAYERED) & ~WS_EX_TRANSPARENT);
            }
        #endif

        // バッファの入れ替え
        glfwSwapBuffers(_window);

        
    }

    Release();

    LAppDelegate::ReleaseInstance();
}

LAppDelegate::LAppDelegate():
    _cubismOption(),
    _window(NULL),
    _captured(false),
    _mouseX(0.0f),
    _mouseY(0.0f),
    _isEnd(false),
    _windowWidth(0),
    _windowHeight(0),
    _isDragging(false),  
    _dragStartX(0.0),    
    _dragStartY(0.0),   
    _bubbleExpireTime(0.0),
    _bubbleModelAnchorX(0.0f),   // 锚点的基准X坐标（模型中心）
    _bubbleModelAnchorY(0.5f),   // 锚点的基准Y坐标（头顶附近）
    _bubbleModelOffsetX(0.1f),   // 在模型坐标系里，向右偏移多少（0.3大概是半个头的宽度）
    _bubbleModelOffsetY(0.1f)    // 在模型坐标系里，向上偏移多少
{

    _isSearchBoxOpen = false;                 // 默认关着
    memset(_searchBuffer, 0, sizeof(_searchBuffer)); // 把缓冲区清空

    _view = new LAppView();
    _textureManager = new LAppTextureManager();
}

LAppDelegate::~LAppDelegate()
{

}

void LAppDelegate::InitializeCubism()
{
    //setup cubism
    _cubismOption.LogFunction = LAppPal::PrintMessage;
    _cubismOption.LoggingLevel = LAppDefine::CubismLoggingLevel;
    _cubismOption.LoadFileFunction = LAppPal::LoadFileAsBytes;
    _cubismOption.ReleaseBytesFunction = LAppPal::ReleaseBytes;
    Csm::CubismFramework::StartUp(&_cubismAllocator, &_cubismOption);

    //Initialize cubism
    CubismFramework::Initialize();

    //load model
    LAppLive2DManager::GetInstance();

    //default proj
    CubismMatrix44 projection;

    LAppPal::UpdateTime();
}


void LAppDelegate::OnMouseCallBack(GLFWwindow* window, int button, int action, int modify)
{
    // 操，把这行删了！ImGui 会自动调用的！
    // ImGui_ImplGlfw_MouseButtonCallback(window, button, action, modify); 

    // 只需要判断 ImGui 是否需要鼠标，如果它不要，我们再处理自己的逻辑
    if (!ImGui::GetIO().WantCaptureMouse)
    {
        if (_view == NULL) return;

        if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
        {
            GetContextMenu().Open();
            return;
        }
        
        if (button == GLFW_MOUSE_BUTTON_LEFT)
        {
            if (action == GLFW_PRESS)
            {
                _captured = true;
                _view->OnTouchesBegan(_mouseX, _mouseY);
            }
            else if (action == GLFW_RELEASE)
            {
                if (_captured)
                {
                    _captured = false;
                    _view->OnTouchesEnded(_mouseX, _mouseY);
                }
            }
        }
    }
}

void LAppDelegate::OnMouseCallBack(GLFWwindow* window, double x, double y)
{
    // 操，也把这行删了！这就是无限递归的罪魁祸首！
    // ImGui_ImplGlfw_CursorPosCallback(window, x, y);

    // 同样，判断 ImGui 是否需要鼠标
    if (!ImGui::GetIO().WantCaptureMouse)
    {
        // 鼠标按下时，是拖拽模型
        if (_captured) 
        {
            const float dx = static_cast<float>(x - _mouseX);
            const float dy = static_cast<float>(y - _mouseY);
            const float viewX = _view->TransformViewX(_mouseX);
            const float viewY = _view->TransformViewY(_mouseY);
            const float newViewX = _view->TransformViewX(_mouseX + dx);
            const float newViewY = _view->TransformViewY(_mouseY + dy);
            const float viewDx = newViewX - viewX;
            const float viewDy = newViewY - viewY;
            _view->GetViewMatrix()->TranslateRelative(viewDx, viewDy);
        }
    
        // 无论鼠标是否按下，都更新视角跟踪
        _view->OnTouchesMoved(static_cast<float>(x), static_cast<float>(y));
    }

    // 无论如何都要更新鼠标坐标，否则当鼠标从ImGui窗口移开时会发生跳变
    _mouseX = static_cast<float>(x);
    _mouseY = static_cast<float>(y);
}


void LAppDelegate::ShowBubbleMessage(const std::string& message, float duration)
{
    _bubbleMessage = message;
    _bubbleExpireTime = glfwGetTime() + duration; 
}


void LAppDelegate::BuildMenuItems()
{
    _contextMenu.Clear(); // 先清空旧菜单

    LAppLive2DManager* live2dManager = LAppLive2DManager::GetInstance();
    if (!live2dManager || live2dManager->GetModel(0) == NULL) return;
    
    LAppModel* model = live2dManager->GetModel(0);
    ICubismModelSetting* modelSetting = model->GetModelSetting();

    // --- 构建“切换表情”这个带子菜单的大项 ---
    MenuItem expressionMenu;
    expressionMenu.Name = "切换表情";

    if (modelSetting)
    {
        for (int i = 0; i < modelSetting->GetExpressionCount(); ++i)
        {
            // 1. 先把指针转成一个 string 对象，把数据拷出来
            std::string expressionName = modelSetting->GetExpressionName(i); 

            expressionMenu.SubItems.push_back({
                expressionName, // 用 string 对象的名字
                // 2. lambda 捕获这个 string 对象，而不是该死的指针！
                [model, expressionName]() { 
                    model->SetExpression(expressionName.c_str()); 
                }
            });
        }
    }
    // 把这个构建好的大项加到主菜单
    _contextMenu.AddItem(expressionMenu);


    // --- 构建“切换动作”菜单 ---
    MenuItem motionMenu;
    motionMenu.Name = "切换动作";

    // Live2D 的动作是按组分的，比如 "Idle", "TapBody" 等等。
    // 我们遍历所有的动作组。
    for (int i = 0; i < modelSetting->GetMotionGroupCount(); ++i)
    {
        const char* groupName = modelSetting->GetMotionGroupName(i);
        
        // 我们可以过滤掉一些不想要的组，比如 "Idle" 组通常是待机动作，
        // 手动触发意义不大。我们就保留那些看起来像是交互动作的。
        // 当然你也可以把所有组都加上。我们这里先全加上，让你看看效果。
        
        // 为每一个动作组创建一个子菜单
        MenuItem groupSubMenu;
        groupSubMenu.Name = groupName;
        
        // 遍历这个组里的所有具体动作
        for (int j = 0; j < modelSetting->GetMotionCount(groupName); ++j)
        {
            // 获取动作文件名 (不带路径)
            const char* motionFileName = modelSetting->GetMotionFileName(groupName, j);
            
            // 为了显示得好看点，我们可以把 .motion3.json 后缀去掉
            std::string motionName = motionFileName;
            size_t pos = motionName.find(".motion3.json");
            if (pos != std::string::npos)
            {
                motionName.erase(pos);
            }
            
            // 把这个具体动作加到组的子菜单里
            groupSubMenu.SubItems.push_back({
                motionName,
                // 点击后，就让模型播放这个动作
                [model, group = std::string(groupName), index = j]() {
                    // StartMotion 需要 组名 和 组内索引
                    model->StartMotion(group.c_str(), index, PriorityNormal);
                }
            });
        }
        
        // 如果这个组里有动作，就把这个组的子菜单加到“切换动作”主菜单项里
        if (!groupSubMenu.SubItems.empty())
        {
            motionMenu.SubItems.push_back(groupSubMenu);
        }
    }
    
    // 如果我们成功找到了任何动作，就把整个“切换动作”菜单加到主菜单栏
    if (!motionMenu.SubItems.empty())
    {
        _contextMenu.AddItem(motionMenu);
    }

    // 2. 加上“快速搜索”
    _contextMenu.AddItem({
        "快速搜索",
        [this]() { 
            _isSearchBoxOpen = true; 
            _searchBoxPos = ImGui::GetMousePos(); // 就加这行，把当前鼠标坐标存起来
            memset(_searchBuffer, 0, sizeof(_searchBuffer));
        }
    });

    _contextMenu.AddItem({
        "与我对话",

        [this]() {
            LAppModel* model = LAppLive2DManager::GetInstance()->GetModel(0);
            LAppView* view = GetView();

            if (model && view)
            {
                // 1. 获取模型真正的、实时的边界！
                const float modelLeft = model->GetModelLeft();
                const float modelTop = model->GetModelTop();

                // 2. 这就是模型真正的左上角在它自己坐标系里的位置！
                const float modelTopLeftX = modelLeft;
                const float modelTopLeftY = modelTop;
                
                // 3. 把这个正确的坐标转换成屏幕坐标
                float screenX, screenY;
                view->ProjectToScreen(modelTopLeftX, modelTopLeftY, screenX, screenY);
                
                // 4. 把算好的屏幕坐标传给 Open 函数
                _chatWindow.Open(ImVec2(screenX, screenY));
            }
            else
            {
                // 兜底方案
                _chatWindow.Open(ImGui::GetMousePos());
            }
        }
    });
    
    _contextMenu.AddItem({ "---" }); // ImGui风格的分隔线，名字是啥不重要

    _contextMenu.AddItem({
        "退出",
        []() { GetInstance()->AppEnd(); }
    });
}


std::string LAppDelegate::GetModelInfoAsJsonString()
{
    using json = nlohmann::json;

    LAppModel* model = LAppLive2DManager::GetInstance()->GetModel(0);
    ICubismModelSetting* modelSetting = model ? model->GetModelSetting() : nullptr;
    if (!modelSetting)
    {
        return "{}";
    }

    json info;
    
    // --- 收集所有表情 ---
    json expressions = json::array();
    for (int i = 0; i < modelSetting->GetExpressionCount(); ++i)
    {
        expressions.push_back(modelSetting->GetExpressionName(i));
    }
    info["expressions"] = expressions;

    // --- 收集所有动作 ---
    json motions = json::object();
    for (int i = 0; i < modelSetting->GetMotionGroupCount(); ++i)
    {
        const char* groupNameCStr = modelSetting->GetMotionGroupName(i);
        std::string groupName = groupNameCStr;
        
        json motion_list = json::array();
        for (int j = 0; j < modelSetting->GetMotionCount(groupNameCStr); ++j)
        {
            std::string motionFileName = modelSetting->GetMotionFileName(groupNameCStr, j);
            size_t pos = motionFileName.find(".motion3.json");
            if (pos != std::string::npos)
            {
                motionFileName.erase(pos);
            }
            motion_list.push_back(motionFileName);
        }
        motions[groupName] = motion_list;
    }
    info["motions"] = motions;

    return info.dump(); // .dump() 会把 json 对象转成字符串
}