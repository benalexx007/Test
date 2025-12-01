#include "text.h"
#include <iostream>

Text::Text(SDL_Renderer* renderer) : renderer(renderer) {}

Text::~Text() { cleanup(); }

bool Text::create(SDL_Renderer* rend, const std::string& fPath, int fSize, const std::string& text, SDL_Color col)
{
    cleanup();
    renderer = rend;
    fontPath = fPath;
    fontSize = fSize;
    currentText = text;
    color = col;

    if (!renderer) { std::cerr << "Text::create - no renderer\n"; return false; }

    font = TTF_OpenFont(fontPath.c_str(), fontSize);
    if (!font) {
        std::cerr << "Text::create - TTF_OpenFont failed for " << fontPath << " | " << TTF_GetError() << "\n";
        return false;
    }

    // build texture for initial text (may be empty)
    return updateTexture();
}

bool Text::updateTexture()
{
    // free old texture
    if (texture) {
        SDL_DestroyTexture(texture);
        texture = nullptr;
        w = h = 0;
    }

    if (currentText.empty()) return true;

    if (!font || !renderer) return false;

    // render to surface then create texture
    SDL_Surface* surf = TTF_RenderUTF8_Blended(font, currentText.c_str(), color);
    if (!surf) {
        std::cerr << "Text::updateTexture - TTF_RenderUTF8_Blended failed | " << TTF_GetError() << "\n";
        return false;
    }

    texture = SDL_CreateTextureFromSurface(renderer, surf);
    if (!texture) {
        std::cerr << "Text::updateTexture - SDL_CreateTextureFromSurface failed | " << SDL_GetError() << "\n";
        SDL_FreeSurface(surf);
        return false;
    }

    w = surf->w;
    h = surf->h;
    SDL_FreeSurface(surf);
    return true;
}

bool Text::setText(const std::string& text)
{
    currentText = text;
    return updateTexture();
}

bool Text::setFontSize(int size)
{
    if (size <= 0) return false;
    fontSize = size;
    if (font) {
        TTF_CloseFont(font);
        font = nullptr;
    }
    font = TTF_OpenFont(fontPath.c_str(), fontSize);
    if (!font) {
        std::cerr << "Text::setFontSize - TTF_OpenFont failed for " << fontPath << " | " << TTF_GetError() << "\n";
        return false;
    }
    return updateTexture();
}

void Text::setColor(SDL_Color c)
{
    color = c;
    updateTexture();
}

void Text::setPosition(int px, int py) { x = px; y = py; }
void Text::moveBy(int dx, int dy) { x += dx; y += dy; }

void Text::render()
{
    if (!renderer || !texture) return;
    SDL_FRect dst = { static_cast<float>(x), static_cast<float>(y), static_cast<float>(w), static_cast<float>(h) };
    SDL_RenderTexture(renderer, texture, NULL, &dst);
}

int Text::getX() const { return x; }
int Text::getY() const { return y; }
int Text::getWidth() const { return w; }
int Text::getHeight() const { return h; }

void Text::cleanup()
{
    if (texture) { SDL_DestroyTexture(texture); texture = nullptr; }
    if (font) { TTF_CloseFont(font); font = nullptr; }
}