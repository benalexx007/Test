#pragma once
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <string>

class Text {
private:
    SDL_Renderer* renderer = nullptr;
    TTF_Font* font = nullptr;
    SDL_Texture* texture = nullptr;
    std::string currentText;
    std::string fontPath;
    SDL_Color color = {255,255,255,255};
    int fontSize = 16;
    int x = 0, y = 0;
    int w = 0, h = 0;

    bool updateTexture();

public:
    Text(SDL_Renderer* renderer = nullptr);
    ~Text();

    // initialize with font file, size and initial text
    bool create(SDL_Renderer* renderer, const std::string& fontPath, int fontSize, const std::string& text = "", SDL_Color color = {255,255,255,255});

    // change content / appearance
    bool setText(const std::string& text);
    bool setFontSize(int size); // recreates font
    void setColor(SDL_Color c);

    // positioning
    void setPosition(int px, int py);
    void moveBy(int dx, int dy);

    // render
    void render();

    // getters
    int getX() const;
    int getY() const;
    int getWidth() const;
    int getHeight() const;

    // free resources
    void cleanup();
};