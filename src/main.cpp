#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include "game.h"
#include "screens/start.h"
#include "core/audio.h"
extern Audio* g_audioInstance = nullptr;

int main(int argc, char** argv) {
    SDL_Window* window = nullptr;
    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();
    // Khởi tạo audio nếu chưa có (để nhạc nền tiếp tục phát)
    if (!g_audioInstance) {
        SDL_Init(SDL_INIT_AUDIO);
        g_audioInstance = new Audio();
        if (g_audioInstance->init()) {
            // Load và phát nhạc nền
            if (g_audioInstance->loadBackgroundMusic("assets/audio/background_music.wav")) {
                g_audioInstance->playBackgroundMusic(true); // Loop
            } else {
                std::cerr << "Failed to load background music\n";
            }
        }
    }
    window = SDL_CreateWindow("Mê Cung Tây Du", 1920, 911, SDL_WINDOW_RESIZABLE);
    SDL_MaximizeWindow(window);
    Start start;
    start.run(window);
    SDL_DestroyWindow(window); 
    window = nullptr;
    if (g_audioInstance) {
        g_audioInstance->cleanup();
        delete g_audioInstance;
        g_audioInstance = nullptr;
    }
    SDL_Quit();
    TTF_Quit();
    return 0;
}