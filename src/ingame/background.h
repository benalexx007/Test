#pragma once
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <string>

class Background {
private:
    SDL_Renderer* renderer = nullptr;
    SDL_Texture* texture = nullptr; // Single background texture instance

public:
    Background(SDL_Renderer* renderer = nullptr);
    ~Background();

    // Load the background texture for the given stage
    // Expected file name: background{stage}.png in the background assets folder
    bool load(char stage);

    // Render the background stretched to the given window size (static)
    void render(int winW, int winH);

    // free texture
    void cleanup();
};