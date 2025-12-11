#include "text.h"
#include <iostream>
#include <algorithm>
#include <cstring>

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

    // ensure wrap alignment is applied if wrap width already set
    if (wrapWidth > 0) {
        TTF_SetFontWrapAlignment(font, TTF_HORIZONTAL_ALIGN_CENTER);
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
        surf = TTF_RenderText_Blended_Wrapped(font, currentText.c_str(), 0, color, wrapWidth);
    } else {
        // single-line blended render
        surf = TTF_RenderText_Blended(font, currentText.c_str(), 0, color);
    }

    if (!surf) {
        std::cerr << "Text::updateTexture - TTF_RenderText... failed | " << SDL_GetError() << "\n";
         return false;
    }

    // Trim empty/transparent columns from the left and right so the resulting texture width
    // matches the visible glyph area. This ensures horizontal centering works as expected
    // when using wrapped text (which often produces a full-width surface).
    int usedLeft = surf->w;
    int usedRight = -1;

    const SDL_PixelFormatDetails* fmt = SDL_GetPixelFormatDetails(surf->format);
    int bpp = fmt ? fmt->bytes_per_pixel : SDL_BYTESPERPIXEL(surf->format);

    Uint8* pixels = static_cast<Uint8*>(surf->pixels);
    const int pitch = surf->pitch;

    SDL_Palette* palette = SDL_GetSurfacePalette(surf);

    for (int x = 0; x < surf->w; ++x) {
        bool columnHasPixel = false;
        for (int y = 0; y < surf->h; ++y) {
            Uint8* p = pixels + y * pitch + x * bpp;
            Uint32 pixel = 0;

            switch (bpp) {
                case 1:
                    pixel = p[0];
                    break;
                case 2:
                    pixel = (Uint32)(p[0]) | ((Uint32)(p[1]) << 8);
                    break;
                case 3:
                    if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
                        pixel = (Uint32)(p[0] << 16) | (Uint32)(p[1] << 8) | (Uint32)p[2];
                    else
                        pixel = (Uint32)p[0] | ((Uint32)p[1] << 8) | ((Uint32)p[2] << 16);
                    break;
                case 4: {
                    Uint32 v;
                    std::memcpy(&v, p, 4);
                    pixel = v;
                    break;
                }
                default:
                    pixel = 0;
                    break;
            }

            Uint8 r = 0, g = 0, b = 0, a = 255;
            if (fmt) {
                SDL_GetRGBA(pixel, fmt, palette, &r, &g, &b, &a);
            } else {
                // fallback: use SDL_GetRGB and treat any non-black as opaque
                const SDL_PixelFormatDetails* tmpFmt = SDL_GetPixelFormatDetails(surf->format);
                SDL_GetRGB(pixel, tmpFmt, palette, &r, &g, &b);
                a = (r || g || b) ? 255 : 0;
            }

            if (a > 0) {
                columnHasPixel = true;
                break;
            }
        }
        if (columnHasPixel) {
            usedLeft = std::min(usedLeft, x);
            usedRight = std::max(usedRight, x);
        }
    }

    SDL_Surface* finalSurf = surf;
    bool cropped = false;

    if (usedRight >= usedLeft) {
        // Need to crop to [usedLeft..usedRight]
        int newW = usedRight - usedLeft + 1;
        SDL_Rect srcRect = { usedLeft, 0, newW, surf->h };
        SDL_Surface* croppedSurf = SDL_CreateSurface(newW, surf->h, surf->format);
        if (croppedSurf) {
            if (SDL_BlitSurface(surf, &srcRect, croppedSurf, NULL) == 0) {
                finalSurf = croppedSurf;
                cropped = true;
            } else {
                SDL_DestroySurface(croppedSurf);
                finalSurf = surf;
            }
            // free original surface if we replaced it
            if (cropped) {
                SDL_DestroySurface(surf);
            }
        } else {
            finalSurf = surf;
        }
    } else {
        finalSurf = surf;
    }

    texture = SDL_CreateTextureFromSurface(renderer, finalSurf);
    if (!texture) {
        std::cerr << "Text::updateTexture - SDL_CreateTextureFromSurface failed | " << SDL_GetError() << "\n";
        if (finalSurf && finalSurf != surf) SDL_DestroySurface(finalSurf);
        if (surf && surf != finalSurf) SDL_DestroySurface(surf);
        return false;
    }

    w = finalSurf->w;
    h = finalSurf->h;

    if (finalSurf && finalSurf != surf && cropped) SDL_DestroySurface(finalSurf);
    else if (surf && surf != finalSurf) SDL_DestroySurface(surf);

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
    if (font && wrapWidth > 0) {
        TTF_SetFontWrapAlignment(font, TTF_HORIZONTAL_ALIGN_CENTER);
    }
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