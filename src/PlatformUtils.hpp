// PlatformUtils.hpp
#pragma once

#include <string>

// 平台相关的工具函数全扔这
namespace PlatformUtils
{
    // 在默认浏览器里打开一个 URL 或者执行搜索查询
    void OpenUrlWithQuery(const std::string& query_utf8);
}