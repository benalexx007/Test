#include "audio.h"
#include <iostream>
#include <cstring>
#include <thread>
#include <chrono>

Audio::Audio() {}

Audio::~Audio() {
    cleanup();
}

bool Audio::init() {
    // SDL audio được init tự động khi mở device
    return true;
}

bool Audio::loadBackgroundMusic(const std::string& filepath) {
    cleanup(); // Cleanup trước nếu có
    
    // Load WAV file
    if (!SDL_LoadWAV(filepath.c_str(), &audioSpec, &audioBuffer, &audioLength)) {
        std::cerr << "Audio::loadBackgroundMusic - Failed to load " << filepath 
                  << ": " << SDL_GetError() << "\n";
        return false;
    }
    
    // Tạo audio stream với format từ WAV file
    audioStream = SDL_OpenAudioDeviceStream(
        SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
        &audioSpec,
        audioCallback,
        this
    );
    
    if (!audioStream) {
        std::cerr << "Audio::loadBackgroundMusic - Failed to open audio stream: " 
                  << SDL_GetError() << "\n";
        SDL_free(audioBuffer);
        audioBuffer = nullptr;
        return false;
    }
    
    audioPosition = 0;
    isPlaying = false;
    
    std::cout << "Audio::loadBackgroundMusic - Loaded " << filepath 
              << " (length: " << audioLength << " bytes)\n";
    return true;
}

void Audio::playBackgroundMusic(bool loop) {
    if (!audioStream || !audioBuffer) {
        std::cerr << "Audio::playBackgroundMusic - No music loaded\n";
        return;
    }
    
    shouldLoop = loop;
    audioPosition = 0;
    
    // Pre-feed một lượng data ban đầu vào stream để tránh underrun
    // Tính toán dựa trên sample rate: ~1 giây audio
    int bytesPerSecond = audioSpec.freq * audioSpec.channels * (SDL_AUDIO_BITSIZE(audioSpec.format) / 8);
    int preBufferSize = bytesPerSecond; // Buffer 1 giây
    
    if (preBufferSize > static_cast<int>(audioLength)) {
        preBufferSize = static_cast<int>(audioLength);
    }
    
    if (preBufferSize > 0 && musicEnabled) {
        SDL_PutAudioStreamData(audioStream, audioBuffer, preBufferSize);
        audioPosition = preBufferSize;
    }
    
    // Resume audio device (SDL_OpenAudioDeviceStream tạo device ở trạng thái paused)
    SDL_ResumeAudioDevice(SDL_GetAudioStreamDevice(audioStream));
    SDL_SetAudioStreamGain(audioStream, 0.2f);
    isPlaying = true;
}

void Audio::setMusicEnabled(bool enabled) {
    musicEnabled = enabled;
    
    if (!audioStream) return;
    
    if (enabled) {
        // Bật nhạc: Resume và set gain = 1.0
        SDL_ResumeAudioDevice(SDL_GetAudioStreamDevice(audioStream));
        SDL_SetAudioStreamGain(audioStream, 0.2f);
        isPlaying = true;
    } else {
        // Tắt nhạc: Set gain = 0 (không pause để giữ vị trí)
        SDL_SetAudioStreamGain(audioStream, 0.0f);
    }
}

bool Audio::isMusicEnabled() const {
    return musicEnabled;
}

void Audio::cleanup() {
    if (audioStream) {
        SDL_DestroyAudioStream(audioStream);
        audioStream = nullptr;
    }
    
    if (audioBuffer) {
        SDL_free(audioBuffer);
        audioBuffer = nullptr;
    }
    
    audioLength = 0;
    audioPosition = 0;
    isPlaying = false;
}

// Static callback function
void Audio::audioCallback(void* userdata, SDL_AudioStream* stream, int additional_amount, int total_amount) {
    Audio* audio = static_cast<Audio*>(userdata);
    if (audio) {
        audio->feedAudioData(additional_amount);
    }
}

void Audio::feedAudioData(int neededBytes) {
    if (!audioStream || !audioBuffer || !musicEnabled) return;
    
    // Chỉ feed đủ data mà stream cần, không feed quá nhiều
    // Giữ buffer ở mức ~0.5-1 giây để tránh rè
    const int maxBufferBytes = 65536; // ~64KB buffer tối đa
    
    // Kiểm tra xem stream đã có đủ data chưa
    int queued = SDL_GetAudioStreamQueued(audioStream);
    if (queued >= maxBufferBytes) {
        // Đã có đủ buffer, không cần feed thêm
        return;
    }
    
    // Tính số bytes cần feed (đủ để đạt maxBufferBytes hoặc theo yêu cầu)
    int bytesToFeed = maxBufferBytes - queued;
    if (neededBytes > 0 && neededBytes < bytesToFeed) {
        bytesToFeed = neededBytes;
    }
    
    // Tính số bytes còn lại trong buffer
    int bytesRemaining = static_cast<int>(audioLength - audioPosition);
    
    if (bytesRemaining <= 0) {
        // Đã hết data
        if (shouldLoop) {
            // Loop: reset về đầu
            audioPosition = 0;
            bytesRemaining = static_cast<int>(audioLength);
        } else {
            // Không loop: dừng
            return;
        }
    }
    
    // Giới hạn số bytes feed dựa trên data còn lại
    if (bytesToFeed > bytesRemaining) {
        bytesToFeed = bytesRemaining;
    }
    
    if (bytesToFeed > 0) {
        const Uint8* dataToFeed = audioBuffer + audioPosition;
        int result = SDL_PutAudioStreamData(audioStream, dataToFeed, bytesToFeed);
        if (result < 0) {
            std::cerr << "Audio::feedAudioData - Failed to put audio data: " 
                      << SDL_GetError() << "\n";
            return;
        }
        
        audioPosition += bytesToFeed;
        
        // Nếu đã hết và cần loop, reset
        if (audioPosition >= audioLength && shouldLoop) {
            audioPosition = 0;
        }
    }
}

bool Audio::playOneShot(const std::string& filepath) {
    SDL_AudioSpec spec;
    Uint8* buf = nullptr;
    Uint32 len = 0;
    if (!SDL_LoadWAV(filepath.c_str(), &spec, &buf, &len)) {
        std::cerr << "Audio::playOneShot - Failed to load " << filepath
                  << ": " << SDL_GetError() << "\n";
        return false;
    }

    SDL_AudioStream* stream = SDL_OpenAudioDeviceStream(
        SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
        &spec,
        nullptr,
        nullptr
    );

    if (!stream) {
        std::cerr << "Audio::playOneShot - Failed to open audio stream: "
                  << SDL_GetError() << "\n";
        SDL_free(buf);
        return false;
    }

    int result = SDL_PutAudioStreamData(stream, buf, static_cast<int>(len));
    if (result < 0) {
        std::cerr << "Audio::playOneShot - Failed to put audio data: " << SDL_GetError() << "\n";
        SDL_DestroyAudioStream(stream);
        SDL_free(buf);
        return false;
    }

    SDL_ResumeAudioDevice(SDL_GetAudioStreamDevice(stream));

    // detached thread will wait until queued data is played and then clean up
    std::thread([stream, buf]() {
        while (SDL_GetAudioStreamQueued(stream) > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        SDL_DestroyAudioStream(stream);
        SDL_free(buf);
    }).detach();

    return true;
}