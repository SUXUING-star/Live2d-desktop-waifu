#include "OllamaClient.hpp"
#include "nlohmann/json.hpp"
#include <iostream>
#include <sstream>

// 不再需要 <stdexcept> 了，因为我们不抛异常了
#include <curl/curl.h>

using json = nlohmann::json;

OllamaClient::OllamaClient() {}

static size_t write_callback(char* ptr, size_t size, size_t nmemb, void* userdata)
{
    auto on_chunk = static_cast<std::function<void(const std::string&)>*>(userdata);
    if (!on_chunk) return 0;

    size_t real_size = size * nmemb;
    std::string data(ptr, real_size);

    std::stringstream ss(data);
    std::string line;
    while (std::getline(ss, line))
    {
        if (line.empty() || line == "\r") continue;
        try 
        {
            json chunk_json = json::parse(line);
            if (chunk_json.contains("message") && chunk_json["message"].contains("content")) 
            {
                std::string content_chunk = chunk_json["message"]["content"];
                (*on_chunk)(content_chunk);
            }
        } 
        catch (const json::parse_error& e)
        {
            std::cerr << "JSON parse error in stream: " << e.what() << "\nInvalid JSON line: " << line << std::endl;
        }
    }
    return real_size;
}

void OllamaClient::StreamChat(const std::vector<Message>& messages,
                              std::function<void(const std::string&)> on_chunk,
                              std::function<void()> on_done)
{
    // --- 操，全局初始化/清理放到主程序里去做，这里只管创建和销毁句柄 ---
    // curl_global_init(CURL_GLOBAL_ALL); // 删掉，这玩意应该在程序启动时调用一次就行
    
    CURL* curl = curl_easy_init();
    if (!curl) {
        std::cerr << "[OllamaClient] Failed to initialize curl" << std::endl;
        on_done(); // 初始化失败，直接调用 on_done 返回
        return;
    }

    struct curl_slist *headers = NULL;
    CURLcode res;

    try {
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
        
        headers = curl_slist_append(headers, "Content-Type: application/json");
        
        curl_easy_setopt(curl, CURLOPT_URL, _apiUrl.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request_body.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &on_chunk);
        
        // 设置5秒连接超时和30秒总超时
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L); 
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);

        res = curl_easy_perform(curl);
        
        // --- 操，核心修改在这里：不再抛异常，而是打印错误 ---
        if (res != CURLE_OK) {
            std::cerr << "[OllamaClient] curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
            // 即使出错了，我们也要继续往下走，调用 on_done
        }
    }
    catch (const std::exception& e) {
        // 捕获一些意外的错误，比如json构建失败
        std::cerr << "[OllamaClient] Exception during request setup: " << e.what() << std::endl;
    }

    // --- 无论成功、失败还是异常，最终都会执行到这里 ---
    
    // 1. 调用 on_done，通知调用方“我这边完事了”
    //    如果网络成功，on_done 会处理收到的数据
    //    如果网络失败，on_done 会处理一个空的数据，然后静默失败
    on_done();

    // 2. 清理资源
    if (headers) {
        curl_slist_free_all(headers);
    }
    curl_easy_cleanup(curl);
    
    // curl_global_cleanup(); // 删掉，这玩意应该在程序退出时调用一次
}