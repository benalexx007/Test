#include "audio.h"
#include <iostream>
#include <cstring>
#include <thread>
#include <chrono>
Audio::Audio() {}

Audio::~Audio() {
    cleanup();
}

// The wrapper's init function is intentionally lightweight. SDL's
// audio subsystem is initialized implicitly when opening a device or
// stream; therefore no explicit global initialization is necessary
// here beyond ensuring that SDL is available.
bool Audio::init() {
    return true;
}

// Load a WAV file into memory and prepare an SDL audio stream for
// subsequent playback. Any previously loaded music is first cleaned
// up to avoid resource leaks.
bool Audio::loadBackgroundMusic(const std::string& filepath) {
    cleanup(); // Ensure no previous resources remain

    // Load WAV file into memory and obtain its format description.
    if (!SDL_LoadWAV(filepath.c_str(), &audioSpec, &audioBuffer, &audioLength)) {
        std::cerr << "Audio::loadBackgroundMusic - Failed to load " << filepath 
                  << ": " << SDL_GetError() << "\n";
        return false;
    }
    
    // Create an audio stream bound to the default playback device.
    // The static `audioCallback` will be used to feed the stream.
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

// Begin playback of the loaded background music. The function pre-fills
// the audio stream with approximately one second of audio to avoid an
// initial underrun, then resumes the device. The `loop` parameter
// controls whether the music should restart when reaching the end.
void Audio::playBackgroundMusic(bool loop) {
    if (!audioStream || !audioBuffer) {
        std::cerr << "Audio::playBackgroundMusic - No music loaded\n";
        return;
    }
    
    shouldLoop = loop;
    audioPosition = 0;
    
    // Pre-feed approximately one second of audio data based on the
    // sample rate and channel count to reduce the risk of underrun.
    int bytesPerSecond = audioSpec.freq * audioSpec.channels * (SDL_AUDIO_BITSIZE(audioSpec.format) / 8);
    int preBufferSize = bytesPerSecond; // Buffer ~1 second
    
    if (preBufferSize > static_cast<int>(audioLength)) {
        preBufferSize = static_cast<int>(audioLength);
    }
    
    if (preBufferSize > 0 && musicEnabled) {
        SDL_PutAudioStreamData(audioStream, audioBuffer, preBufferSize);
        audioPosition = preBufferSize;
    }
    
    // Resume audio device; set a conservative gain to avoid loud output
    // on some systems.
    SDL_ResumeAudioDevice(SDL_GetAudioStreamDevice(audioStream));
    SDL_SetAudioStreamGain(audioStream, 0.2f);
    isPlaying = true;
}

// Enable or disable music output. When disabling music the code sets
// stream gain to zero rather than pausing the device, preserving the
// playback position for a seamless resume.
void Audio::setMusicEnabled(bool enabled) {
    musicEnabled = enabled;
    
    if (!audioStream) return;
    
    if (enabled) {
        SDL_ResumeAudioDevice(SDL_GetAudioStreamDevice(audioStream));
        SDL_SetAudioStreamGain(audioStream, 0.2f);
        isPlaying = true;
    } else {
        SDL_SetAudioStreamGain(audioStream, 0.0f);
    }
}

bool Audio::isMusicEnabled() const {
    return musicEnabled;
}

// Free allocated audio resources and reset bookkeeping fields. This
// function is safe to call multiple times.
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

// Static callback invoked by the SDL audio subsystem. It delegates
// to the instance method `feedAudioData` which implements buffer
// management and looping semantics.
void Audio::audioCallback(void* userdata, SDL_AudioStream* stream, int additional_amount, int total_amount) {
    Audio* audio = static_cast<Audio*>(userdata);
    if (audio) {
        audio->feedAudioData(additional_amount);
    }
}

// Feed audio data into the audio stream on demand. The implementation
// keeps the queued stream bytes within reasonable bounds to avoid
// underruns while also preventing excessive memory usage.
void Audio::feedAudioData(int neededBytes) {
    if (!audioStream || !audioBuffer || !musicEnabled) return;
    
    // Maximum number of bytes to keep queued in the stream buffer.
    const int maxBufferBytes = 65536; // ~64KB
    
    // Determine how many bytes are currently queued. If the queued
    // amount meets or exceeds our target, do not add more.
    int queued = SDL_GetAudioStreamQueued(audioStream);
    if (queued >= maxBufferBytes) {
        return;
    }
    
    // Decide how many bytes to write: the lesser of the requested
    // amount and the amount needed to reach the target buffer size.
    int bytesToFeed = maxBufferBytes - queued;
    if (neededBytes > 0 && neededBytes < bytesToFeed) {
        bytesToFeed = neededBytes;
    }
    
    // Compute how many bytes remain in the loaded audio buffer.
    int bytesRemaining = static_cast<int>(audioLength - audioPosition);
    
    if (bytesRemaining <= 0) {
        if (shouldLoop) {
            // For looping playback, wrap the position to the start.
            audioPosition = 0;
            bytesRemaining = static_cast<int>(audioLength);
        } else {
            // Non-looping playback with no data remaining: nothing to do.
            return;
        }
    }
    
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
        
        // Wrap to the beginning if looping is enabled and we've reached the end.
        if (audioPosition >= audioLength && shouldLoop) {
            audioPosition = 0;
        }
    }
}

// Play a single-shot WAV file by opening a transient audio stream and
// writing the file's contents into it. A detached thread monitors the
// stream's queued bytes and performs cleanup once playback completes.
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

    // Spawn a detached thread that monitors the stream until all queued
    // data has been consumed; the thread then destroys the stream and
    // frees the temporary buffer. This avoids blocking the main loop.
    std::thread([stream, buf]() {
        while (SDL_GetAudioStreamQueued(stream) > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        SDL_DestroyAudioStream(stream);
        SDL_free(buf);
    }).detach();

    return true;
}