#pragma once
#include <SDL3/SDL.h>
#include <string>

class Audio {
public:
    Audio();
    ~Audio();
    
    // Khởi tạo hệ thống audio
    bool init();
    
    // Load và phát nhạc nền
    // Lưu ý: SDL3 chỉ hỗ trợ WAV trực tiếp, MP3 cần decoder hoặc convert sang WAV
    bool loadBackgroundMusic(const std::string& filepath);
    void playBackgroundMusic(bool loop = true);

    // Play a one-shot WAV effect (non-looping). Returns true on success.
    bool playOneShot(const std::string& filepath);
    
    // Bật/tắt nhạc nền (không stop, chỉ set volume)
    void setMusicEnabled(bool enabled);
    bool isMusicEnabled() const;
    
    // Cleanup
    void cleanup();

private:
    SDL_AudioStream* audioStream = nullptr;
    SDL_AudioDeviceID audioDevice = 0;
    Uint8* audioBuffer = nullptr;
    Uint32 audioLength = 0;
    Uint32 audioPosition = 0;
    SDL_AudioSpec audioSpec;
    bool musicEnabled = true;
    bool isPlaying = false;
    bool shouldLoop = true;
    
    // Callback để cung cấp audio data
    static void audioCallback(void* userdata, SDL_AudioStream* stream, int additional_amount, int total_amount);
    void feedAudioData(int neededBytes = 0);
};