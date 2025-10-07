// src/OllamaClient.cpp
#include "OllamaClient.hpp"
#include "nlohmann/json.hpp"
#include <iostream>
#include <sstream>

// 操！我们要直接用 curl 的 C API！
#include <curl/curl.h>

using json = nlohmann::json;

OllamaClient::OllamaClient() {}

// 这是 libcurl 的数据写入回调函数
// 每次收到一小块数据，这个函数就会被调用
static size_t write_callback(char* ptr, size_t size, size_t nmemb, void* userdata)
{
    // userdata 就是我们传进去的 on_chunk 函数
    auto on_chunk = static_cast<std::function<void(const std::string&)>*>(userdata);
    
    // 我们收到的数据大小是 size * nmemb
    size_t real_size = size * nmemb;
    std::string data(ptr, real_size);

    // 把收到的原始数据（可能包含多个JSON对象）切分成一行一行的
    std::stringstream ss(data);
    std::string line;
    while (std::getline(ss, line))
    {
        if (line.empty() || line == "\r") continue;
        try 
        {
            // 解析每一行 JSON
            json chunk_json = json::parse(line);
            if (chunk_json.contains("message") && chunk_json["message"].contains("content")) 
            {
                std::string content_chunk = chunk_json["message"]["content"];
                // 真正地、实时地把一小块文本块通过回调传出去！
                (*on_chunk)(content_chunk);
            }
        } 
        catch (const json::parse_error& e)
        {
            std::cerr << "JSON parse error in stream: " << e.what() << "\nInvalid JSON line: " << line << std::endl;
        }
    }

    return real_size; // 必须返回处理的数据大小
}

void OllamaClient::StreamChat(const std::vector<Message>& messages,
                              std::function<void(const std::string&)> on_chunk,
                              std::function<void()> on_done)
{
    CURL* curl;
    CURLcode res;

    // 初始化 libcurl
    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
    if(curl) {
        // --- 1. 准备请求数据 ---
        json messages_json = json::array();
        for (const auto& msg : messages) {
            messages_json.push_back({{"role", msg.role}, {"content", msg.content}});
        }
        json request_body_json = {
            {"model", _model},
            {"messages", messages_json},
            {"stream", true}
        };
        std::string request_body = request_body_json.dump();
        
        // --- 2. 设置 HTTP Header ---
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");

        // --- 3. 配置 curl 句柄 ---
        curl_easy_setopt(curl, CURLOPT_URL, _apiUrl.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request_body.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        // 关键！设置写入回调函数
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        // 关键！把 on_chunk 函数的指针传给回调
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &on_chunk);

        // --- 4. 执行请求 ---
        // 这是一个同步阻塞调用，但因为我们设置了回调，
        // 它会在收到数据的过程中，不断地调用我们的 write_callback。
        // 这就是真·流式处理！
        res = curl_easy_perform(curl);
        
        // --- 5. 检查错误并清理 ---
        if(res != CURLE_OK) {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        }

        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
    }
    curl_global_cleanup();
    
    // 当 curl_easy_perform() 返回时，意味着整个流已经结束
    on_done();
}