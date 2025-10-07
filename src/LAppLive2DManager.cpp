/**
 * Copyright(c) Live2D Inc. All rights reserved.
 *
 * Use of this source code is governed by the Live2D Open Software license
 * that can be found at https://www.live2d.com/eula/live2d-open-software-license-agreement_en.html.
 */

#include "LAppLive2DManager.hpp"
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <io.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <Rendering/CubismRenderer.hpp>
#include "LAppPal.hpp"
#include "LAppDefine.hpp"
#include "LAppDelegate.hpp"
#include "LAppModel.hpp"
#include "LAppView.hpp"

using namespace Csm;
using namespace LAppDefine;
using namespace std;

namespace {
    LAppLive2DManager* s_instance = NULL;

    void BeganMotion(ACubismMotion* self)
    {
        LAppPal::PrintLogLn("Motion Began: %x", self);
    }

    void FinishedMotion(ACubismMotion* self)
    {
        LAppPal::PrintLogLn("Motion Finished: %x", self);
    }

    int CompareCsmString(const void* a, const void* b)
    {
        return strcmp(reinterpret_cast<const Csm::csmString*>(a)->GetRawString(),
            reinterpret_cast<const Csm::csmString*>(b)->GetRawString());
    }
}

LAppLive2DManager* LAppLive2DManager::GetInstance()
{
    if (s_instance == NULL)
    {
        s_instance = new LAppLive2DManager();
    }

    return s_instance;
}

void LAppLive2DManager::ReleaseInstance()
{
    if (s_instance != NULL)
    {
        delete s_instance;
    }

    s_instance = NULL;
}

LAppLive2DManager::LAppLive2DManager()
    : _viewMatrix(NULL)
    , _sceneIndex(0)
    , _isModelLoadedAndNeedResize(false)
    , _tapCount(0)      // 初始化
    , _lastTapTime(0.0f) // 初始化
{
    _viewMatrix = new CubismMatrix44();
    // SetUpModel();

    // ChangeScene(_sceneIndex);

    // 1. 定义模型文件夹名
    const csmString modelDirName = "IceGIrl"; 

    // 2. 构造文件路径
    csmString modelPath(ResourcesPath);
    modelPath += modelDirName;
    modelPath.Append(1, '/'); // 加上斜杠, e.g., "Resources/Haru/"

    csmString modelJsonName(modelDirName);
    modelJsonName += ".model3.json"; // e.g., "Haru.model3.json"
    
    // 3. 加载模型
    if (DebugLogEnable)
    {
        LAppPal::PrintLogLn("[APP]Load model: %s", modelDirName.GetRawString());
    }
    ReleaseAllModel(); // 先清空，以防万一
    _models.PushBack(new LAppModel());
    _models[0]->LoadAssets(modelPath.GetRawString(), modelJsonName.GetRawString());

    

}

LAppLive2DManager::~LAppLive2DManager()
{
    ReleaseAllModel();
    delete _viewMatrix;
}

void LAppLive2DManager::ReleaseAllModel()
{
    for (csmUint32 i = 0; i < _models.GetSize(); i++)
    {
        delete _models[i];
    }

    _models.Clear();
}

void LAppLive2DManager::SetUpModel()
{
    // ResourcesPathの中にあるフォルダ名を全てクロールし、モデルが存在するフォルダを定義する。
    // フォルダはあるが同名の.model3.jsonが見つからなかった場合はリストに含めない。
    // 一部文字が受け取れないためワイド文字で受け取ってUTF8に変換し格納する。

    csmString crawlPath(ResourcesPath);
    crawlPath += "*.*";

    wchar_t wideStr[MAX_PATH];
    csmChar name[MAX_PATH];
    LAppPal::ConvertMultiByteToWide(crawlPath.GetRawString(), wideStr, MAX_PATH);

    struct _wfinddata_t fdata;
    intptr_t fh = _wfindfirst(wideStr, &fdata);
    if (fh == -1)
    {
        return;
    }

    _modelDir.Clear();

    while (_wfindnext(fh, &fdata) == 0)
    {
        if ((fdata.attrib & _A_SUBDIR) && wcscmp(fdata.name, L"..") != 0)
        {
            LAppPal::ConvertWideToMultiByte(fdata.name, name, MAX_PATH);

            // フォルダと同名の.model3.jsonがあるか探索する
            csmString model3jsonPath(ResourcesPath);
            model3jsonPath += name;
            model3jsonPath.Append(1, '/');
            model3jsonPath += name;
            model3jsonPath += ".model3.json";

            LAppPal::ConvertMultiByteToWide(model3jsonPath.GetRawString(), wideStr, MAX_PATH);

            struct _wfinddata_t fdata2;
            if (_wfindfirst(wideStr, &fdata2) != -1)
            {
                _modelDir.PushBack(csmString(name));
            }
        }
    }
    qsort(_modelDir.GetPtr(), _modelDir.GetSize(), sizeof(csmString), CompareCsmString);
}

csmVector<csmString> LAppLive2DManager::GetModelDir() const
{
    return _modelDir;
}

csmInt32 LAppLive2DManager::GetModelDirSize() const
{
    return _modelDir.GetSize();
}

LAppModel* LAppLive2DManager::GetModel(csmUint32 no) const
{
    if (no < _models.GetSize())
    {
        return _models[no];
    }

    return NULL;
}

void LAppLive2DManager::OnDrag(csmFloat32 x, csmFloat32 y) const
{
    for (csmUint32 i = 0; i < _models.GetSize(); i++)
    {
        LAppModel* model = GetModel(i);

        model->SetDragging(x, y);
    }
}

void LAppLive2DManager::OnTap(csmFloat32 x, csmFloat32 y)
{
    if (DebugLogEnable)
    {
        LAppPal::PrintLogLn("[APP]tap point: {x:%.2f y:%.2f}", x, y);
    }

    if (_models.GetSize() == 0) return;

    LAppModel* model = GetModel(0);
    if (!model) return;

    // --- 核心在这儿：用他妈的正确API进行坐标逆向转换 ---

    // 1. 获取模型自身的矩阵
    CubismMatrix44* modelMatrix = model->GetModelMatrix();

    // 2. 使用正确的API，将视图坐标(x,y)转换为模型本地坐标(motionX, motionY)
    //    InvertTransformX 就是干这个的！
    const float motionX = modelMatrix->InvertTransformX(x);
    const float motionY = modelMatrix->InvertTransformY(y);

    // 3. 获取模型画布的原始宽高 (这个尺寸是模型内部定义的，通常是 -1.0 到 1.0)
    const csmFloat32 canvasWidth = model->GetModel()->GetCanvasWidth();
    const csmFloat32 canvasHeight = model->GetModel()->GetCanvasHeight();

    // 4. 定义头部和身体的大概范围 (在模型本地坐标系里)
    //    - 模型本地坐标系的原点(0,0)在模型中心
    //    - Y轴向上为正，向下为负
    //    - 我们就简单粗暴地认为：Y > 0 就是头，Y <= 0 就是身体
    const float head_min_y = 0.0f;

    float currentTime = LAppPal::GetTotalTime();

    // 1. 如果距离上次点击超过0.5秒，就算连击断了，计数器清零
    if (currentTime - _lastTapTime > 0.5f)
    {
        _tapCount = 0;
    }

    // 2. 只要连击没断，每次点击都无条件 +1
    _tapCount++;
    
    // 3. 更新最后一次点击的时间
    _lastTapTime = currentTime;

    LAppPal::PrintLogLn("[APP] Tap Combo: %d", _tapCount); // <<<=== 加上这行日志，方便你看计数

    // 4. 判断是否触发气泡
    if (_tapCount >= 5)
    {
        const char* messages[] = { "别点我了","手不累吗？" };
        int randomIndex = rand() % 2;
        LAppDelegate::GetInstance()->ShowBubbleMessage(messages[randomIndex], 3.0f);
        _tapCount = 0; // 触发后清零，免得一直弹
    }

    // 5. 开始判断！
    // 只要点击了模型身体，就触发随机对话
    if (motionX >= -canvasWidth / 2.0f && motionX <= canvasWidth / 2.0f &&
        motionY >= -canvasHeight / 2.0f && motionY <= canvasHeight / 2.0f)
    {
        if (motionY > head_min_y) // 点了头，还是换表情
        {
            if (DebugLogEnable) LAppPal::PrintLogLn("[APP]hit area: [Head]");
            model->SetRandomExpression();
        }
        else // 点了身体，触发随机对话
        {
            if (DebugLogEnable) LAppPal::PrintLogLn("[APP]hit area: [Body - Triggering Talk]");
            LAppDelegate::GetInstance()->GetRandomTalker()->TriggerRandomTalk();
        }
    }
}
void LAppLive2DManager::OnUpdate() const
{
    int width, height;
    glfwGetWindowSize(LAppDelegate::GetInstance()->GetWindow(), &width, &height);

    csmUint32 modelCount = _models.GetSize();
    for (csmUint32 i = 0; i < modelCount; ++i)
    {
        CubismMatrix44 projection;
        LAppModel* model = GetModel(i);

        if (model->GetModel() == NULL)
        {
            LAppPal::PrintLogLn("Failed to model->GetModel().");
            continue;
        }

        if (model->GetModel()->GetCanvasWidth() > 1.0f && width < height)
        {
            // 横に長いモデルを縦長ウィンドウに表示する際モデルの横サイズでscaleを算出する
            model->GetModelMatrix()->SetWidth(2.0f);
            projection.Scale(1.0f, static_cast<float>(width) / static_cast<float>(height));
        }
        else
        {
            projection.Scale(static_cast<float>(height) / static_cast<float>(width), 1.0f);
        }

        // 必要があればここで乗算
        if (_viewMatrix != NULL)
        {
            projection.MultiplyByMatrix(_viewMatrix);
        }

        // モデル1体描画前コール
        LAppDelegate::GetInstance()->GetView()->PreModelDraw(*model);

        model->Update();
        model->Draw(projection);///< 参照渡しなのでprojectionは変質する

        // モデル1体描画後コール
        LAppDelegate::GetInstance()->GetView()->PostModelDraw(*model);
    }
}

void LAppLive2DManager::NextScene()
{
    csmInt32 no = (_sceneIndex + 1) % GetModelDirSize();
    ChangeScene(no);
}

void LAppLive2DManager::ChangeScene(Csm::csmInt32 index)
{
    _sceneIndex = index;
    if (DebugLogEnable)
    {
        LAppPal::PrintLogLn("[APP]model index: %d", _sceneIndex);
    }

    // model3.jsonのパスを決定する.
    // ディレクトリ名とmodel3.jsonの名前を一致していることが条件
    const csmString& model = _modelDir[index];

    csmString modelPath(ResourcesPath);
    modelPath += model;
    modelPath.Append(1, '/');

    csmString modelJsonName(model);
    modelJsonName += ".model3.json";

    ReleaseAllModel();
    _models.PushBack(new LAppModel());
    _models[0]->LoadAssets(modelPath.GetRawString(), modelJsonName.GetRawString());

    /*
     * モデル半透明表示を行うサンプルを提示する。
     * ここでUSE_RENDER_TARGET、USE_MODEL_RENDER_TARGETが定義されている場合
     * 別のレンダリングターゲットにモデルを描画し、描画結果をテクスチャとして別のスプライトに張り付ける。
     */
    {
#if defined(USE_RENDER_TARGET)
        // LAppViewの持つターゲットに描画を行う場合、こちらを選択
        LAppView::SelectTarget useRenderTarget = LAppView::SelectTarget_ViewFrameBuffer;
#elif defined(USE_MODEL_RENDER_TARGET)
        // 各LAppModelの持つターゲットに描画を行う場合、こちらを選択
        LAppView::SelectTarget useRenderTarget = LAppView::SelectTarget_ModelFrameBuffer;
#else
        // デフォルトのメインフレームバッファへレンダリングする(通常)
        LAppView::SelectTarget useRenderTarget = LAppView::SelectTarget_None;
#endif

#if defined(USE_RENDER_TARGET) || defined(USE_MODEL_RENDER_TARGET)
        // モデル個別にαを付けるサンプルとして、もう1体モデルを作成し、少し位置をずらす
        _models.PushBack(new LAppModel());
        _models[1]->LoadAssets(modelPath.GetRawString(), modelJsonName.GetRawString());
        _models[1]->GetModelMatrix()->TranslateX(0.2f);
#endif

        LAppDelegate::GetInstance()->GetView()->SwitchRenderingTarget(useRenderTarget);

        // 別レンダリング先を選択した際の背景クリア色
        float clearColor[3] = { 0.0f, 0.0f, 0.0f };
        LAppDelegate::GetInstance()->GetView()->SetRenderTargetClearColor(clearColor[0], clearColor[1], clearColor[2]);
    }
}

csmUint32 LAppLive2DManager::GetModelNum() const
{
    return _models.GetSize();
}

void LAppLive2DManager::SetViewMatrix(CubismMatrix44* m)
{
    for (int i = 0; i < 16; i++) {
        _viewMatrix->GetArray()[i] = m->GetArray()[i];
    }
}
