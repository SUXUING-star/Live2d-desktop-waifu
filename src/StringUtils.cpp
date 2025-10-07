// StringUtils.cpp
#include "StringUtils.hpp"
#include <windows.h>

namespace StringUtils
{
    std::wstring Utf8ToWide(const std::string& str_utf8)
    {
        if (str_utf8.empty()) {
            return L"";
        }
        
        int wide_len = MultiByteToWideChar(CP_UTF8, 0, str_utf8.c_str(), -1, NULL, 0);
        if (wide_len == 0) {
            return L"";
        }
        
        std::wstring result(wide_len, 0);
        MultiByteToWideChar(CP_UTF8, 0, str_utf8.c_str(), -1, &result[0], wide_len);

        if (!result.empty() && result.back() == L'\0') {
           result.pop_back();
        }
        
        return result;
    }
}