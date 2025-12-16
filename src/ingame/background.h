#pragma once
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <string>

class Background {
private:
    SDL_Renderer* renderer = nullptr;
    SDL_Texture* textures[3] = { nullptr };
    int bgIndex = 0;
    Uint32 bgLastChange = 0;
    Uint32 bgDelayMs = 500; // ms between frames

public:
    Background(SDL_Renderer* renderer = nullptr);
    ~Background();

    // load textures for stage (background{stage}_1..3.png)
    bool load(char stage);

    // render current background stretched to window size
    void render(int winW, int winH);

    // free textures
    void cleanup();

};