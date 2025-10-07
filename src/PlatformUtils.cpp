// PlatformUtils.cpp
#include "PlatformUtils.hpp"
#include "StringUtils.hpp"      // <-- 需要用到我们的字符串转换工具
#include <windows.h>
#include <shellapi.h>

namespace PlatformUtils
{
    void OpenUrlWithQuery(const std::string& query_utf8)
    {
    #if defined(_WIN32)
        std::string search_url_utf8 = "https://www.bing.com/search?q=" + query_utf8;

        std::wstring search_url_wide = StringUtils::Utf8ToWide(search_url_utf8);

        ShellExecuteW(
            NULL,
            L"open",
            search_url_wide.c_str(),
            NULL,
            NULL,
            SW_SHOWNORMAL
        );
    #endif
    }
}