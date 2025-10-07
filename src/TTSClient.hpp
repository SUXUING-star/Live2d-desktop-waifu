// src/TTSClient.hpp
#pragma once
#include <string>
#include <thread>
#include <vector>
#include <atomic>
#include <mutex>
#include <condition_variable>

// FFmpeg 头文件需要 extern "C"
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
#include <libavutil/opt.h> // 我操！这次我绝对不会忘了！
#include <libavutil/channel_layout.h> // 操！这个也得加！
}

#include "miniaudio.h"

class TTSClient
{
public: // 操！直接改成 public，再报错我自宫！
    std::mutex _pcmMutex;
    std::vector<uint8_t> _pcmBuffer;

public:
    TTSClient();
    ~TTSClient();
    void Speak(const std::string& text, const std::string& language);
    void Stop();

private:
    void AudioThread(std::string text, std::string language);
    static int ReadPacket(void* opaque, uint8_t* buf, int buf_size);

    const std::string _apiUrl = "http://127.0.0.1:9880";
    
    ma_device _device;
    AVIOContext* _avioContext = nullptr;
    AVFormatContext* _formatContext = nullptr;
    AVCodecContext* _codecContext = nullptr;
    SwrContext* _swrContext = nullptr;
    AVFrame* _frame = nullptr;

    std::vector<uint8_t> _networkBuffer;
    std::mutex _networkMutex;
    std::condition_variable _networkCv;
    size_t _networkCursor = 0;
    std::atomic<bool> _networkFinished;
    
    std::thread _audioThread;
    std::atomic<bool> _shouldStop;
};