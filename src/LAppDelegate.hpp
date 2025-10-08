/**
 * Copyright(c) Live2D Inc. All rights reserved.
 *
 * Use of this source code is governed by the Live2D Open Software license
 * that can be found at https://www.live2d.com/eula/live2d-open-software-license-agreement_en.html.
 */

#pragma once

#include "RandomTalker.hpp" 
#include "ImGuiBubble.hpp"
#include "ContextMenu.hpp"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "LAppAllocator_Common.hpp"
#include "imgui.h"
#include "ChatWindow.hpp" 
#include "OllamaClient.hpp"
#include "TTSClient.hpp"

class LAppView;
class LAppTextureManager;

/**
* @brief   アプリケーションクラス。
*   Cubism SDK の管理を行う。
*/
class LAppDelegate
{
public:
    /**
    * @brief   クラスのインスタンス（シングルトン）を返す。<br>
    *           インスタンスが生成されていない場合は内部でインスタンを生成する。
    *
    * @return  クラスのインスタンス
    */
    static LAppDelegate* GetInstance();

    /**
    * @brief   クラスのインスタンス（シングルトン）を解放する。
    *
    */
    static void ReleaseInstance();

    /**
    * @brief   APPに必要なものを初期化する。
    */
    bool Initialize();

    /**
    * @brief   解放する。
    */
    void Release();

    /**
    * @brief   実行処理。
    */
    void Run();

    /**
    * @brief   OpenGL用 glfwSetMouseButtonCallback用関数。
    *
    * @param[in]       window            コールバックを呼んだWindow情報
    * @param[in]       button            ボタン種類
    * @param[in]       action            実行結果
    * @param[in]       modify
    */
    void OnMouseCallBack(GLFWwindow* window, int button, int action, int modify);

    /**
    * @brief   OpenGL用 glfwSetCursorPosCallback用関数。
    *
    * @param[in]       window            コールバックを呼んだWindow情報
    * @param[in]       x                 x座標
    * @param[in]       y                 x座標
    */
    void OnMouseCallBack(GLFWwindow* window, double x, double y);

    /**
    * @brief   Window情報を取得する。
    */
    GLFWwindow* GetWindow() { return _window; }

    /**
    * @brief   View情報を取得する。
    */
    LAppView* GetView() { return _view; }

    /**
    * @brief   アプリケーションを終了するかどうか。
    */
    bool GetIsEnd() { return _isEnd; }

    /**
    * @brief   アプリケーションを終了する。
    */
    void AppEnd() { _isEnd = true; }

    LAppTextureManager* GetTextureManager() { return _textureManager; }

    void ShowBubbleMessage(const std::string& message, float duration);

    ContextMenu& GetContextMenu() { return _contextMenu; } 

    void BuildMenuItems();

    std::string GetModelInfoAsJsonString();
    
    RandomTalker* GetRandomTalker() { return _randomTalker; }
    OllamaClient* GetOllamaClient() { return _ollamaClient; }
    TTSClient*    GetTTSClient()    { return _ttsClient; }

private:
    /**
    * @brief   コンストラクタ
    */
    LAppDelegate();

    /**
    * @brief   デストラクタ
    */
    ~LAppDelegate();

    /**
    * @brief   Cubism SDK の初期化
    */
    void InitializeCubism();

    LAppAllocator_Common _cubismAllocator;              ///< Cubism SDK Allocator
    Csm::CubismFramework::Option _cubismOption;  ///< Cubism SDK Option
    GLFWwindow* _window;                         ///< OpenGL ウィンドウ
    LAppView* _view;                             ///< View情報
    bool _captured;                              ///< クリックしているか
    float _mouseX;                               ///< マウスX座標
    float _mouseY;                               ///< マウスY座標
    bool _isEnd;                                 ///< APP終了しているか
    LAppTextureManager* _textureManager;         ///< テクスチャマネージャー

    int _windowWidth;                            ///< Initialize関数で設定したウィンドウ幅
    int _windowHeight;                           ///< Initialize関数で設定したウィンドウ高さ

    bool _isDragging;       // 是否正在拖动窗口
    double _dragStartX;     // 开始拖动时的鼠标X坐标
    double _dragStartY;     // 开始拖动时的鼠标Y坐标

    ImGuiBubble _bubble; 
    std::string _bubbleMessage;
    double      _bubbleExpireTime;

    float _bubbleModelAnchorX; // 模型锚点X
    float _bubbleModelAnchorY; // 模型锚点Y
    float _bubbleModelOffsetX; // 屏幕像素偏移X
    float _bubbleModelOffsetY; // 屏幕像素偏移Y

    ContextMenu _contextMenu;
    ChatWindow _chatWindow; 

    bool _isSearchBoxOpen;           // 控制搜索框显示的开关
    char _searchBuffer[256];         // 存搜索文字的缓冲区

    ImVec2 _searchBoxPos;  // 存储搜索框应该出现的位置
    RandomTalker*   _randomTalker; // 核心随机对话对象
    OllamaClient* _ollamaClient;
    TTSClient*    _ttsClient;
    
};

class EventHandler
{
public:
    /**
    * @brief   glfwSetMouseButtonCallback用コールバック関数。
    */
    static void OnMouseCallBack(GLFWwindow* window, int button, int action, int modify)
    {
        LAppDelegate::GetInstance()->OnMouseCallBack(window, button, action, modify);
    }

    /**
    * @brief   glfwSetCursorPosCallback用コールバック関数。
    */
    static void OnMouseCallBack(GLFWwindow* window, double x, double y)
    {
         LAppDelegate::GetInstance()->OnMouseCallBack(window, x, y);
    }

};
