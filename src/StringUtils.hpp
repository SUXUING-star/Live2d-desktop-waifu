// StringUtils.hpp
#pragma once

#include <string>

// 把咱们的工具函数声明放这里
// 以后有别的字符串相关的破事，也往这里加
namespace StringUtils
{
    // 把 UTF-8 字符串转换成 Windows API 能用的宽字符串
    std::wstring Utf8ToWide(const std::string& str_utf8);
}