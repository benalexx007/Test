#include "background.h"
#include <iostream>

Background::Background(SDL_Renderer* renderer) : renderer(renderer) {}

Background::~Background() { cleanup(); }

bool Background::load(const std::string& stage)
{
    if (!renderer) return false;
    for (int i = 0; i < 3; ++i) {
        char index = '1' + i;
        std::string path = "assets/images/background/background" + stage + "_" + index + ".png";
        textures[i] = IMG_LoadTexture(renderer, path.c_str());
        if (!textures[i]) {
            std::cerr << "Failed to load background: " << path << " | " << SDL_GetError() << std::endl;
        }
    }
    bgIndex = 0;
    bgLastChange = SDL_GetTicks();
    return true;
}

void Background::render(int winW, int winH)
{
    // no renderer or textures => nothing to draw
    if (!renderer) return;

    // advance animation non-blocking
    Uint32 now = SDL_GetTicks();
    if (now - bgLastChange >= bgDelayMs) {
        bgIndex = (bgIndex + 1) % 3;
        bgLastChange = now;
    }

    SDL_Texture* tex = textures[bgIndex];
    if (!tex) return;

    SDL_FRect dst = { 0.0f, 0.0f, static_cast<float>(winW), static_cast<float>(winH) };
    SDL_RenderTexture(renderer, tex, NULL, &dst);
}

void Background::cleanup()
{
    for (int i = 0; i < 3; ++i) {
        if (textures[i]) {
            SDL_DestroyTexture(textures[i]);
            textures[i] = nullptr;
        }
    }
}