/**
 * Copyright(c) Live2D Inc. All rights reserved.
 *
 * Use of this source code is governed by the Live2D Open Software license
 * that can be found at https://www.live2d.com/eula/live2d-open-software-license-agreement_en.html.
 */

#include "LAppSpriteShader.hpp"
#include <Utils/CubismDebug.hpp>
#include "LAppDefine.hpp"
#include "LAppPal.hpp"

using namespace LAppDefine;

LAppSpriteShader::LAppSpriteShader()
{
    _programId = CreateShader();
}

LAppSpriteShader::~LAppSpriteShader()
{
    glDeleteShader(_programId);
}

GLuint LAppSpriteShader::GetShaderId() const
{
    return _programId;
}

GLuint LAppSpriteShader::CreateShader()
{
    // 顶点着色器源码
    const GLchar* vertexShaderSource =
        "#version 120\n"
        "attribute vec4 position;"
        "attribute vec2 uv;"
        "varying vec2 vuv;"
        "void main(void){"
        "   gl_Position = position;"
        "   vuv = uv;"
        "}";

    // 片段着色器源码
    const GLchar* fragmentShaderSource =
        "#version 120\n"
        "varying vec2 vuv;"
        "uniform sampler2D texture;"
        "uniform vec4 baseColor;"
        "void main(void){"
        "   gl_FragColor = texture2D(texture, vuv) * baseColor;"
        "}";
    
    // --- 下面的逻辑是标准的编译和链接流程 ---
    
    // 编译顶点着色器
    GLuint vertexShaderId = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShaderId, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShaderId);
    if (!CheckShader(vertexShaderId)) // 用你已有的函数检查编译错误
    {
        glDeleteShader(vertexShaderId);
        CubismLogError("Vertex shader compile error!");
        return 0;
    }

    // 编译片段着色器
    GLuint fragmentShaderId = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShaderId, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShaderId);
    if (!CheckShader(fragmentShaderId)) // 用你已有的函数检查编译错误
    {
        glDeleteShader(vertexShaderId);
        glDeleteShader(fragmentShaderId);
        CubismLogError("Fragment shader compile error!");
        return 0;
    }

    // 链接成一个程序
    GLuint programId = glCreateProgram();
    glAttachShader(programId, vertexShaderId);
    glAttachShader(programId, fragmentShaderId);
    glLinkProgram(programId);

    // 检查链接错误
    GLint linkStatus;
    glGetProgramiv(programId, GL_LINK_STATUS, &linkStatus);
    if (linkStatus == GL_FALSE)
    {
        GLint logLength;
        glGetProgramiv(programId, GL_INFO_LOG_LENGTH, &logLength);
        if (logLength > 0)
        {
            GLchar* log = (GLchar*)CSM_MALLOC(logLength);
            glGetProgramInfoLog(programId, logLength, &logLength, log);
            CubismLogError("Shader link log: %s", log);
            CSM_FREE(log);
        }
        glDeleteProgram(programId);
        return 0;
    }

    // 用一下这个程序，以确保某些驱动的懒加载机制生效
    glUseProgram(programId);

    // 链接完成后就可以删除单个shader对象了
    glDeleteShader(vertexShaderId);
    glDeleteShader(fragmentShaderId);

    return programId;
}

bool LAppSpriteShader::CheckShader(GLuint shaderId)
{
    GLint status;
    GLint logLength;
    glGetShaderiv(shaderId, GL_INFO_LOG_LENGTH, &logLength);
    if (logLength > 0)
    {
        GLchar* log = reinterpret_cast<GLchar*>(CSM_MALLOC(logLength));
        glGetShaderInfoLog(shaderId, logLength, &logLength, log);
        CubismLogError("Shader compile log: %s", log);
        CSM_FREE(log);
    }

    glGetShaderiv(shaderId, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE)
    {
        glDeleteShader(shaderId);
        return false;
    }

    return true;
}

GLuint LAppSpriteShader::CompileShader(Csm::csmString filename, GLenum shaderType)
{
    // ファイル読み込み
    Csm::csmSizeInt bufferSize = 0;
    const char* shaderString = reinterpret_cast<const char*>(LAppPal::LoadFileAsBytes(filename.GetRawString(), &bufferSize));
    const GLint glSize = (GLint)bufferSize;

    // コンパイル
    GLuint shaderId = glCreateShader(shaderType);
    glShaderSource(shaderId, 1, &shaderString, &glSize);
    glCompileShader(shaderId);

    // 読み込んだシェーダー文字列の開放
    LAppPal::ReleaseBytes(reinterpret_cast<Csm::csmByte*>(const_cast<char*>(shaderString)));

    if (!CheckShader(shaderId))
    {
        return 0;
    }

    return shaderId;
}
