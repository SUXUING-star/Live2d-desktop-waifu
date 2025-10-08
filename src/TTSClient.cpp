#define MINIAUDIO_IMPLEMENTATION
#define NOMINMAX
#include "TTSClient.hpp"
#include <cpr/cpr.h>
#include "nlohmann/json.hpp"
#include <iostream>
#include <algorithm>

using json = nlohmann::json;

void pcm_data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
    TTSClient* client = (TTSClient*)pDevice->pUserData;
    if (!client) return;

    // --- 操，核心修改在这里 ---
    
    // 我们将要从 _pcmBuffer 中读取的数据，同时也会是我们要计算音量的数据
    uint8_t* data_to_process = nullptr; 
    size_t bytes_to_process = 0;

    // 1. 先从缓冲区取出即将播放的数据
    {
        std::lock_guard<std::mutex> lock(client->_pcmMutex);
        
        size_t bytesToRead = frameCount * ma_get_bytes_per_frame(pDevice->playback.format, pDevice->playback.channels);
        
        if (client->_pcmBuffer.size() >= bytesToRead) {
            memcpy(pOutput, client->_pcmBuffer.data(), bytesToRead);
            data_to_process = client->_pcmBuffer.data(); // 指向这块数据
            bytes_to_process = bytesToRead;
            client->_pcmBuffer.erase(client->_pcmBuffer.begin(), client->_pcmBuffer.begin() + bytesToRead);
        } else {
            size_t remaining_bytes = client->_pcmBuffer.size();
            if (remaining_bytes > 0) {
                 memcpy(pOutput, client->_pcmBuffer.data(), remaining_bytes);
                 data_to_process = client->_pcmBuffer.data();
                 bytes_to_process = remaining_bytes;
                 client->_pcmBuffer.clear();
            }
            if (bytesToRead > remaining_bytes) {
                // 用静音填充剩余部分
                memset((uint8_t*)pOutput + remaining_bytes, 0, bytesToRead - remaining_bytes);
            }
        }
    }

    // 2. 如果我们成功取到了数据，就计算它的音量
    if (data_to_process && bytes_to_process > 0) {
        int16_t* samples = reinterpret_cast<int16_t*>(data_to_process);
        int sample_count = bytes_to_process / sizeof(int16_t);
        
        double sum_sq = 0.0;
        for (int i = 0; i < sample_count; ++i) {
            double sample_val = static_cast<double>(samples[i]) / 32768.0;
            sum_sq += sample_val * sample_val;
        }
        double rms = (sample_count > 0) ? sqrt(sum_sq / sample_count) : 0.0;
        
        // 3. 更新全局音量变量 (这个回调本身就在音频线程，但为了安全还是加锁)
        client->UpdateVolume(static_cast<float>(rms));
    } else {
        // 如果没数据了（播放结束），就把音量设为0
        client->UpdateVolume(0.0f);
    }
}

// --- 辅助函数 (不变) ---
static AVSampleFormat ma_format_to_av_sample_fmt(ma_format format) {
    switch (format) {
        case ma_format_u8:  return AV_SAMPLE_FMT_U8;
        case ma_format_s16: return AV_SAMPLE_FMT_S16;
        case ma_format_s32: return AV_SAMPLE_FMT_S32;
        case ma_format_f32: return AV_SAMPLE_FMT_FLT;
        default:            return AV_SAMPLE_FMT_NONE;
    }
}

// --- 构造/析构/控制函数 ---
TTSClient::TTSClient() : _shouldStop(false), _networkFinished(true) 
{
    memset(&_device, 0, sizeof(ma_device));
}

TTSClient::~TTSClient() 
{ 
    Stop(); 
}

void TTSClient::Stop() {
    _shouldStop = true;
    _networkCv.notify_all();
   if (_audioThread.joinable()) {
        _audioThread.detach(); 
    }
}

void TTSClient::Speak(const std::string& text, const std::string& language) {
    // 1. 先调用 Stop()，它会设置 _shouldStop = true 并 detach 旧线程
    // 这一步很快，不会阻塞
    Stop(); 
    
    // 2. 重置状态给新线程用
    _shouldStop = false;
    
    // 3. 清理播放缓冲区
    {
        std::lock_guard<std::mutex> lock(_pcmMutex);
        _pcmBuffer.clear();
    }
    
    // --- 操，核心修改在这里 ---
    // 4. 在这里主动把音量归零！
    {
        std::lock_guard<std::mutex> lock(_volumeMutex);
        _currentVolume = 0.0f;
    }

    // 5. 创建并启动新线程
    // 如果上一个线程还没死透，没关系，它的 _shouldStop 已经是 true，
    // 它会在循环的某个检查点退出，不会和新线程冲突。
    _audioThread = std::thread(&TTSClient::AudioThread, this, text, language);
}

// --- FFmpeg 回调 (不变) ---
int TTSClient::ReadPacket(void* opaque, uint8_t* buf, int buf_size) {
    auto* client = static_cast<TTSClient*>(opaque);
    std::unique_lock<std::mutex> lock(client->_networkMutex);

    while (client->_networkCursor >= client->_networkBuffer.size() && !client->_networkFinished) {
        if (client->_shouldStop) return AVERROR_EOF;
        client->_networkCv.wait_for(lock, std::chrono::milliseconds(100));
    }
    
    if (client->_networkFinished && client->_networkBuffer.empty()) {
        return AVERROR_EOF;
    }
    
    if (client->_networkCursor >= client->_networkBuffer.size() && client->_networkFinished) {
        return AVERROR_EOF;
    }
    
    int bytes_to_read = (std::min)(buf_size, (int)(client->_networkBuffer.size() - client->_networkCursor));
    if (bytes_to_read <= 0) return AVERROR(EAGAIN);

    memcpy(buf, client->_networkBuffer.data() + client->_networkCursor, bytes_to_read);
    client->_networkCursor += bytes_to_read;

    return bytes_to_read;
}

// --- 统一资源清理函数 ---
void TTSClient::CleanupFFmpeg()
{
    if (ma_device_is_started(&_device)) {
        ma_device_uninit(&_device);
        memset(&_device, 0, sizeof(ma_device));
    }
    if (_packet) { av_packet_free(&_packet); _packet = nullptr; }
    if (_frame) { av_frame_free(&_frame); _frame = nullptr; }
    if (_swrContext) { swr_free(&_swrContext); _swrContext = nullptr; }
    if (_codecContext) { avcodec_free_context(&_codecContext); _codecContext = nullptr; }
    if (_formatContext) { 
        avformat_close_input(&_formatContext);
        _avioContext = nullptr; // avformat_close_input 释放了它
        _formatContext = nullptr;
    } else if (_avioContext) {
        if (_avioContext->buffer) { av_freep(&_avioContext->buffer); }
        av_freep(&_avioContext); 
        _avioContext = nullptr;
    }
}


// --- 核心音频处理线程 (完整版) ---
void TTSClient::AudioThread(std::string text, std::string language) {
    // Phase 1: Network Request
    _networkFinished = false;
    {
        std::lock_guard<std::mutex> lock(_networkMutex);
        _networkBuffer.clear();
        _networkCursor = 0;
    }
    
    std::thread network_thread([this, text, language]() {
        json request_body = {{"text", text}, {"text_language", language}};
        
        cpr::Response r = cpr::Post(
            cpr::Url{_apiUrl},
            cpr::Body{request_body.dump()},
            cpr::Header{{"Content-Type", "application/json"}},
            cpr::ConnectTimeout{5000},
            cpr::LowSpeed{1024, 10}
        );

        if (r.error || r.status_code != 200) {
            std::cerr << "[TTSClient] Network Error: " << r.error.message << " (Status code: " << r.status_code << ")" << std::endl;
        } else {
            std::lock_guard<std::mutex> lock(_networkMutex);
            _networkBuffer.assign(r.text.begin(), r.text.end());
        }
        
        _networkFinished = true;
        _networkCv.notify_one();
    });
    network_thread.detach();

    // Phase 2: FFmpeg & Miniaudio Initialization
    const int avio_buffer_size = 8192;
    unsigned char* avio_buffer = (unsigned char*)av_malloc(avio_buffer_size);
    if (!avio_buffer) { CleanupFFmpeg(); return; }
    
    _avioContext = avio_alloc_context(avio_buffer, avio_buffer_size, 0, this, &TTSClient::ReadPacket, nullptr, nullptr);
    if (!_avioContext) { /* av_free(avio_buffer) handled by cleanup */ CleanupFFmpeg(); return; }

    _formatContext = avformat_alloc_context();
    if (!_formatContext) { CleanupFFmpeg(); return; }
    _formatContext->pb = _avioContext;
    
    if (avformat_open_input(&_formatContext, nullptr, nullptr, nullptr) != 0) {
        std::cerr << "[TTSClient] avformat_open_input failed. Likely due to network failure." << std::endl;
        CleanupFFmpeg(); return;
    }
    if (avformat_find_stream_info(_formatContext, nullptr) < 0) { CleanupFFmpeg(); return; }

    int stream_index = av_find_best_stream(_formatContext, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
    if (stream_index < 0) { CleanupFFmpeg(); return; }
    
    AVStream* audio_stream = _formatContext->streams[stream_index];
    const AVCodec* codec = avcodec_find_decoder(audio_stream->codecpar->codec_id);
    _codecContext = avcodec_alloc_context3(codec);
    if (!_codecContext) { CleanupFFmpeg(); return; }
    if (avcodec_parameters_to_context(_codecContext, audio_stream->codecpar) < 0) { CleanupFFmpeg(); return; }
    if (avcodec_open2(_codecContext, codec, nullptr) < 0) { CleanupFFmpeg(); return; }

    ma_device_config deviceConfig;
    deviceConfig = ma_device_config_init(ma_device_type_playback);
    deviceConfig.playback.format = ma_format_s16;
    deviceConfig.playback.channels = _codecContext->ch_layout.nb_channels;
    deviceConfig.sampleRate = _codecContext->sample_rate;
    deviceConfig.dataCallback = pcm_data_callback;
    deviceConfig.pUserData = this;

    _swrContext = swr_alloc();
    if (!_swrContext) { CleanupFFmpeg(); return; }

    AVChannelLayout out_ch_layout;
    av_channel_layout_default(&out_ch_layout, deviceConfig.playback.channels);
    av_opt_set_chlayout(_swrContext, "in_chlayout", &_codecContext->ch_layout, 0);
    av_opt_set_int(_swrContext, "in_sample_rate", _codecContext->sample_rate, 0);
    av_opt_set_sample_fmt(_swrContext, "in_sample_fmt", _codecContext->sample_fmt, 0);
    av_opt_set_chlayout(_swrContext, "out_chlayout", &out_ch_layout, 0);
    av_opt_set_int(_swrContext, "out_sample_rate", deviceConfig.sampleRate, 0);
    av_opt_set_sample_fmt(_swrContext, "out_sample_fmt", ma_format_to_av_sample_fmt(deviceConfig.playback.format), 0);
    
    if (swr_init(_swrContext) < 0) {
        av_channel_layout_uninit(&out_ch_layout);
        CleanupFFmpeg(); return;
    }
    av_channel_layout_uninit(&out_ch_layout);
    
    // Phase 3: Playback & Decoding Loop
    if (ma_device_init(NULL, &deviceConfig, &_device) != MA_SUCCESS) { CleanupFFmpeg(); return; }
    ma_device_start(&_device);

    _packet = av_packet_alloc();
    _frame = av_frame_alloc();
    if (!_packet || !_frame) { CleanupFFmpeg(); return; }
    
    while (av_read_frame(_formatContext, _packet) >= 0 && !_shouldStop) {
        if (_packet->stream_index == stream_index) {
            if (avcodec_send_packet(_codecContext, _packet) == 0) {
                while (avcodec_receive_frame(_codecContext, _frame) == 0 && !_shouldStop) {
                    uint8_t* out_buffer = nullptr;
                    av_samples_alloc(&out_buffer, NULL, deviceConfig.playback.channels, _frame->nb_samples, ma_format_to_av_sample_fmt(deviceConfig.playback.format), 0);
                    if (out_buffer) {
                        int converted_samples = swr_convert(_swrContext, &out_buffer, _frame->nb_samples, (const uint8_t**)_frame->data, _frame->nb_samples);
                        
                        if(converted_samples > 0) {
                            int data_size_bytes = converted_samples * deviceConfig.playback.channels * ma_get_bytes_per_sample(deviceConfig.playback.format);
                            std::lock_guard<std::mutex> lock(_pcmMutex);
                            _pcmBuffer.insert(_pcmBuffer.end(), out_buffer, out_buffer + data_size_bytes);
                        }
                        av_freep(&out_buffer);
                    
                    }
                }
            }
        }
        av_packet_unref(_packet);
    }

    while(!_shouldStop) {
        {
            std::lock_guard<std::mutex> lock(_pcmMutex);
            if (_pcmBuffer.empty()) break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }


    // Phase 4: Final Cleanup
    CleanupFFmpeg();
}

// --- 估算时长函数 (不变) ---
float TTSClient::EstimateDuration(const std::string& text) {
    const float chars_per_second = 7.0f; 
    return static_cast<float>(text.length()) / chars_per_second;
}

float TTSClient::GetCurrentVolume() const
{
    std::lock_guard<std::mutex> lock(_volumeMutex);
    return _currentVolume;
}

void TTSClient::UpdateVolume(float rms)
{
    std::lock_guard<std::mutex> lock(_volumeMutex);
    _currentVolume = rms * 5.0f; // 放大倍数不变
    if (_currentVolume > 1.0f) _currentVolume = 1.0f;

    // 加日志确认一下
    static int cb_frame_count = 0;
    if (cb_frame_count++ % 10 == 0 && _currentVolume > 0.01) { // 每10次回调打一次，不刷屏
        // std::cout << "[TTSClient PLAYBACK DEBUG] Volume updated to: " << _currentVolume << std::endl;
    }
}