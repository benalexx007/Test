#pragma once
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <string>

class Background {
private:
    SDL_Renderer* renderer = nullptr;
    SDL_Texture* texture = nullptr; // Chỉ 1 background duy nhất

public:
    Background(SDL_Renderer* renderer = nullptr);
    ~Background();

    // load textures cho stage (background{stage}.png)
    bool load(char stage);

    // render background (tĩnh)
    void render(int winW, int winH);

    // free texture
    void cleanup();
};