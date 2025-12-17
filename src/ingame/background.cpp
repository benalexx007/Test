#include "background.h"
#include <iostream>

Background::Background(SDL_Renderer* renderer) : renderer(renderer) {}

Background::~Background() { cleanup(); }

bool Background::load(char stage)
{
    if (!renderer) return false;

    cleanup(); // Xoá texture cũ nếu có

    // Đường dẫn: assets/images/background/background{stage}.png
    std::string path = "assets/images/background/background" + std::string(1, stage) + ".png";
    texture = IMG_LoadTexture(renderer, path.c_str());
    if (!texture) {
        std::cerr << "Failed to load background: " << path << " | " << SDL_GetError() << std::endl;
        return false;
    }
    return true;
}

void Background::render(int winW, int winH)
{
    // no renderer or texture => nothing to draw
    if (!renderer || !texture) return;

    SDL_FRect dst = { 0.0f, 0.0f, static_cast<float>(winW), static_cast<float>(winH) };
    SDL_RenderTexture(renderer, texture, NULL, &dst);
}

void Background::cleanup()
{
    if (texture) {
        SDL_DestroyTexture(texture);
        texture = nullptr;
    }
}