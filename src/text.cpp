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
    wrapWidth = 0;

    if (!renderer) { std::cerr << "Text::create - no renderer\n"; return false; }

    font = TTF_OpenFont(fontPath.c_str(), fontSize);
    if (!font) {
        std::cerr << "Text::create - TTF_OpenFont failed for " << fontPath << " | " << SDL_GetError() << "\n";
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

    SDL_Surface* surf = nullptr;
    if (wrapWidth > 0) {
        // use wrapped rendering when wrapWidth set
        // SDL_ttf (SDL3) blended wrapped signature:
        // TTF_RenderText_Blended_Wrapped(TTF_Font *font, const char *text, size_t length, SDL_Color fg, int wrap_width)
        surf = TTF_RenderText_Blended_Wrapped(font, currentText.c_str(), 0, color, wrapWidth);
    } else {
        // single-line blended render
        // SDL_ttf (SDL3) blended signature:
        // TTF_RenderText_Blended(TTF_Font *font, const char *text, size_t length, SDL_Color fg)
        surf = TTF_RenderText_Blended(font, currentText.c_str(), 0, color);
    }

    if (!surf) {
        std::cerr << "Text::updateTexture - TTF_RenderText... failed | " << SDL_GetError() << "\n";
         return false;
    }

    texture = SDL_CreateTextureFromSurface(renderer, surf);
    if (!texture) {
        std::cerr << "Text::updateTexture - SDL_CreateTextureFromSurface failed | " << SDL_GetError() << "\n";
        SDL_DestroySurface(surf);
        return false;
    }

    w = surf->w;
    h = surf->h;
    SDL_DestroySurface(surf);
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
        std::cerr << "Text::setFontSize - TTF_OpenFont failed for " << fontPath << " | " << SDL_GetError() << "\n";
         return false;
    }
    return updateTexture();
}

void Text::setColor(SDL_Color c)
{
    color = c;
    updateTexture();
}

void Text::setWrapWidth(int px)
{
    wrapWidth = px > 0 ? px : 0;
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