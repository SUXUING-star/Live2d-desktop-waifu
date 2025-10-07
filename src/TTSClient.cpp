// src/TTSClient.cpp
#define MINIAUDIO_IMPLEMENTATION
#define NOMINMAX
#include "TTSClient.hpp"
#include <cpr/cpr.h>
#include "nlohmann/json.hpp"
#include <iostream>
#include <algorithm>

using json = nlohmann::json;

// miniaudio 回调
void pcm_data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
    TTSClient* client = (TTSClient*)pDevice->pUserData;
    if (!client) return;
    std::lock_guard<std::mutex> lock(client->_pcmMutex);
    
    size_t bytesToRead = frameCount * ma_get_bytes_per_frame(pDevice->playback.format, pDevice->playback.channels);
    if (client->_pcmBuffer.size() >= bytesToRead) {
        memcpy(pOutput, client->_pcmBuffer.data(), bytesToRead);
        client->_pcmBuffer.erase(client->_pcmBuffer.begin(), client->_pcmBuffer.begin() + bytesToRead);
    } else {
        size_t remaining_bytes = client->_pcmBuffer.size();
        if (remaining_bytes > 0) {
             memcpy(pOutput, client->_pcmBuffer.data(), remaining_bytes);
             client->_pcmBuffer.clear();
        }
        if (bytesToRead > remaining_bytes) {
            memset((uint8_t*)pOutput + remaining_bytes, 0, bytesToRead - remaining_bytes);
        }
    }
}

static AVSampleFormat ma_format_to_av_sample_fmt(ma_format format) {
    switch (format) {
        case ma_format_u8:  return AV_SAMPLE_FMT_U8;
        case ma_format_s16: return AV_SAMPLE_FMT_S16;
        case ma_format_s32: return AV_SAMPLE_FMT_S32;
        case ma_format_f32: return AV_SAMPLE_FMT_FLT;
        default:            return AV_SAMPLE_FMT_NONE;
    }
}

TTSClient::TTSClient() : _shouldStop(false), _networkFinished(true) {}
TTSClient::~TTSClient() { Stop(); }

void TTSClient::Stop() {
    _shouldStop = true;
    _networkCv.notify_all();
    if (_audioThread.joinable()) {
        _audioThread.join();
    }
}

void TTSClient::Speak(const std::string& text, const std::string& language) {
    Stop();
    _shouldStop = false;
    {
        std::lock_guard<std::mutex> lock(_pcmMutex);
        _pcmBuffer.clear();
    }
    _audioThread = std::thread(&TTSClient::AudioThread, this, text, language);
}

int TTSClient::ReadPacket(void* opaque, uint8_t* buf, int buf_size) {
    auto* client = static_cast<TTSClient*>(opaque);
    std::unique_lock<std::mutex> lock(client->_networkMutex);

    while (client->_networkCursor >= client->_networkBuffer.size() && !client->_networkFinished) {
        if (client->_shouldStop) return AVERROR_EOF;
        client->_networkCv.wait_for(lock, std::chrono::milliseconds(100));
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

void TTSClient::AudioThread(std::string text, std::string language) {
    // Phase 1: Network
    _networkFinished = false;
    {
        std::lock_guard<std::mutex> lock(_networkMutex);
        _networkBuffer.clear();
        _networkCursor = 0;
    }
    std::thread network_thread([this, text, language]() {
        json request_body = {{"text", text}, {"text_language", language}};
        cpr::Post(cpr::Url{_apiUrl}, cpr::Body{request_body.dump()}, cpr::Header{{"Content-Type", "application/json"}},
                  cpr::WriteCallback{[this](const std::string& data, intptr_t) -> bool {
                      if (_shouldStop) return false;
                      {
                          std::lock_guard<std::mutex> lock(_networkMutex);
                          _networkBuffer.insert(_networkBuffer.end(), data.begin(), data.end());
                      }
                      _networkCv.notify_one();
                      return true;
                  }});
        _networkFinished = true;
        _networkCv.notify_one();
    });
    network_thread.detach();

    // Phase 2: FFmpeg Init
    const int avio_buffer_size = 8192;
    unsigned char* avio_buffer = (unsigned char*)av_malloc(avio_buffer_size);
    if (!avio_buffer) return;
    _avioContext = avio_alloc_context(avio_buffer, avio_buffer_size, 0, this, &TTSClient::ReadPacket, nullptr, nullptr);
    if (!_avioContext) { av_free(avio_buffer); return; }

    _formatContext = avformat_alloc_context();
    if (!_formatContext) goto cleanup;
    _formatContext->pb = _avioContext;
    if (avformat_open_input(&_formatContext, nullptr, nullptr, nullptr) != 0) goto cleanup;
    if (avformat_find_stream_info(_formatContext, nullptr) < 0) goto cleanup;

    int stream_index = av_find_best_stream(_formatContext, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
    if (stream_index < 0) goto cleanup;
    
    AVStream* audio_stream = _formatContext->streams[stream_index];
    const AVCodec* codec = avcodec_find_decoder(audio_stream->codecpar->codec_id);
    _codecContext = avcodec_alloc_context3(codec);
    if (!_codecContext) goto cleanup;
    avcodec_parameters_to_context(_codecContext, audio_stream->codecpar);
    if (avcodec_open2(_codecContext, codec, nullptr) < 0) goto cleanup;

    // Phase 3: miniaudio Init
    ma_device_config deviceConfig;
    deviceConfig = ma_device_config_init(ma_device_type_playback);
    deviceConfig.playback.format = ma_format_s16;
    // 操！这次我严格按照你给的 API 定义来写！
    deviceConfig.playback.channels = _codecContext->ch_layout.nb_channels;
    deviceConfig.sampleRate = _codecContext->sample_rate;
    deviceConfig.dataCallback = pcm_data_callback;
    deviceConfig.pUserData = this;

    // Phase 4: SwrContext Init
    _swrContext = swr_alloc();
    if (!_swrContext) goto cleanup;

    av_opt_set_chlayout(_swrContext, "in_chlayout", &_codecContext->ch_layout, 0);
    av_opt_set_int(_swrContext, "in_sample_rate", _codecContext->sample_rate, 0);
    av_opt_set_sample_fmt(_swrContext, "in_sample_fmt", _codecContext->sample_fmt, 0);
    
    AVChannelLayout out_ch_layout;
    // 操！这次我把参数传对了！
    av_channel_layout_default(&out_ch_layout, deviceConfig.playback.channels);
    av_opt_set_chlayout(_swrContext, "out_chlayout", &out_ch_layout, 0);
    av_opt_set_int(_swrContext, "out_sample_rate", deviceConfig.sampleRate, 0);
    av_opt_set_sample_fmt(_swrContext, "out_sample_fmt", ma_format_to_av_sample_fmt(deviceConfig.playback.format), 0);
    
    if (swr_init(_swrContext) < 0) {
        av_channel_layout_uninit(&out_ch_layout);
        goto cleanup;
    }
    av_channel_layout_uninit(&out_ch_layout);

    // Phase 5: Playback & Decoding Loop
    if (ma_device_init(NULL, &deviceConfig, &_device) != MA_SUCCESS) goto cleanup;
    ma_device_start(&_device);

    AVPacket* packet = av_packet_alloc();
    _frame = av_frame_alloc();
    if (!packet || !_frame) goto cleanup;
    
    while (av_read_frame(_formatContext, packet) >= 0 && !_shouldStop) {
        if (packet->stream_index == stream_index) {
            if (avcodec_send_packet(_codecContext, packet) == 0) {
                while (avcodec_receive_frame(_codecContext, _frame) == 0) {
                    uint8_t* out_buffer = nullptr;
                    // 操！这次我把参数传对了！
                    av_samples_alloc(&out_buffer, NULL, deviceConfig.playback.channels, _frame->nb_samples, ma_format_to_av_sample_fmt(deviceConfig.playback.format), 0);

                    int converted_samples = swr_convert(_swrContext, &out_buffer, _frame->nb_samples, (const uint8_t**)_frame->data, _frame->nb_samples);
                    
                    if(converted_samples > 0) {
                        std::lock_guard<std::mutex> lock(_pcmMutex);
                        int data_size = converted_samples * deviceConfig.playback.channels * ma_get_bytes_per_sample(deviceConfig.playback.format);
                        _pcmBuffer.insert(_pcmBuffer.end(), out_buffer, out_buffer + data_size);
                    }
                    av_freep(&out_buffer);
                }
            }
        }
        av_packet_unref(packet);
    }

    while(!_shouldStop) {
        {
            std::lock_guard<std::mutex> lock(_pcmMutex);
            if (_pcmBuffer.empty()) break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

cleanup:
    _shouldStop = true;
    if (ma_device_is_started(&_device)) {
        ma_device_uninit(&_device);
    }
    if(_frame) { av_frame_free(&_frame); _frame = nullptr; }
    if(packet) { av_packet_free(&packet); packet = nullptr; }
    if(_swrContext) { swr_free(&_swrContext); _swrContext = nullptr; }
    if(_codecContext) { avcodec_free_context(&_codecContext); _codecContext = nullptr; }
    if(_formatContext) { avformat_close_input(&_formatContext); _formatContext = nullptr; }
    else if (_avioContext) {
        if(_avioContext->buffer) { av_freep(&_avioContext->buffer); }
        av_freep(&_avioContext); _avioContext = nullptr;
    }
}