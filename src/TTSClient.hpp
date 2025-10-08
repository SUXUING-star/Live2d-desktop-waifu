#pragma once
#include <string>
#include <thread>
#include <vector>
#include <atomic>
#include <mutex>
#include <condition_variable>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
#include <libavutil/opt.h>
#include <libavutil/channel_layout.h>
}

#include "miniaudio.h"

class TTSClient
{
public:
    std::mutex _pcmMutex; // 保持 public，回调需要
    std::vector<uint8_t> _pcmBuffer; // 保持 public，回调需要

public:
    TTSClient();
    ~TTSClient();

    void Speak(const std::string& text, const std::string& language);
    void Stop();
    float EstimateDuration(const std::string& text);
    float GetCurrentVolume() const;
    void UpdateVolume(float rms);

private:
    void AudioThread(std::string text, std::string language);
    static int ReadPacket(void* opaque, uint8_t* buf, int buf_size);
    void CleanupFFmpeg(); // <-- 操，新增一个专门的清理函数

    const std::string _apiUrl = "http://127.0.0.1:9880";
    
    // FFmpeg 和 miniaudio 的资源
    ma_device _device;
    AVIOContext* _avioContext = nullptr;
    AVFormatContext* _formatContext = nullptr;
    AVCodecContext* _codecContext = nullptr;
    SwrContext* _swrContext = nullptr;
    AVFrame* _frame = nullptr;
    AVPacket* _packet = nullptr; // 把 packet 也作为成员，方便管理

    // 网络相关资源
    std::vector<uint8_t> _networkBuffer;
    std::mutex _networkMutex;
    std::condition_variable _networkCv;
    size_t _networkCursor = 0;
    std::atomic<bool> _networkFinished;
    
    // 线程控制
    std::thread _audioThread;
    std::atomic<bool> _shouldStop;

    mutable std::mutex _volumeMutex;
    float _currentVolume = 0.0f;
};