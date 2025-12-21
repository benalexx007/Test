// Simple audio wrapper that manages background music and one-shot
// sound effects using SDL audio streams. The class encapsulates a
// loaded WAV buffer and provides an internal streaming callback that
// fills the playback stream on demand.
#pragma once
#include <SDL3/SDL.h>
#include <string>

class Audio {
public:
    Audio();
    ~Audio();
    
    // Initialize the audio subsystem wrapper. Currently this method
    // performs no heavy lifting because SDL audio device creation is
    // deferred until a stream is opened.
    bool init();
    
    // Load a background music WAV file into memory and prepare an
    // audio stream for streaming playback. Note: SDL typically only
    // supports WAV natively; compressed formats require an external
    // decoder or prior conversion.
    bool loadBackgroundMusic(const std::string& filepath);
    void playBackgroundMusic(bool loop = true);

    // Play a non-looping one-shot WAV effect. The implementation
    // spawns a detached thread to monitor playback and free resources
    // when the sound has finished playing.
    bool playOneShot(const std::string& filepath);
    
    // Enable or mute background music without destroying the stream
    // so playback position can be preserved when toggling.
    void setMusicEnabled(bool enabled);
    bool isMusicEnabled() const;
    
    // Free allocated audio resources and close streams.
    void cleanup();

private:
    SDL_AudioStream* audioStream = nullptr; // stream for background music
    SDL_AudioDeviceID audioDevice = 0;     // associated device id (if applicable)
    Uint8* audioBuffer = nullptr;          // raw WAV data buffer
    Uint32 audioLength = 0;                // total length of WAV data in bytes
    Uint32 audioPosition = 0;              // current read position in audioBuffer
    SDL_AudioSpec audioSpec;               // WAV file audio specification
    bool musicEnabled = true;              // logical music enabled flag
    bool isPlaying = false;                // whether streaming playback is active
    bool shouldLoop = true;                // whether to loop the background audio
    
    // Static callback invoked by SDL audio stream; it forwards to
    // the instance method `feedAudioData` which performs the actual
    // buffer management.
    static void audioCallback(void* userdata, SDL_AudioStream* stream, int additional_amount, int total_amount);
    void feedAudioData(int neededBytes = 0);
};