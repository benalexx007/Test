#pragma once
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <string>
#include <memory>
#include "../ui/text.h"

class Textbox {
private:
    SDL_Renderer* renderer = nullptr;
    SDL_Texture* bgTexture = nullptr;
    SDL_Rect rect{0,0,0,0};
    
    std::unique_ptr<Text> placeholder;
    std::unique_ptr<Text> inputText;
    std::unique_ptr<Text> cursorText;  // text with cursor appended (for width measurement)
    
    std::string currentInput;
    std::string placeholderStr;
    bool focused = false;
    size_t cursorPos = 0;  // cursor position in the string (0 = before first char)
    
    int fontSize = 72;
    SDL_Color textColor = {0, 0, 0, 255};
    SDL_Color placeholderColor = {150, 150, 150, 255};
    SDL_Color cursorColor = {0, 0, 0, 255};
    std::string fontPath = "assets/font.ttf";
    
    Uint32 lastCursorToggle = 0;  // timestamp of last cursor blink toggle
    bool cursorVisible = true;      // whether cursor is currently visible
    const Uint32 CURSOR_BLINK_MS = 500;  // blink every 500ms
    
    void updateDisplayText();
    void updateCursorBlink();

public:
    Textbox(SDL_Renderer* renderer = nullptr);
    ~Textbox();
    
    bool create(SDL_Renderer* renderer, int x, int y, int w, int h,
                const std::string& bgPath,
                const std::string& placeholderText,
                int fontSize = 72,
                SDL_Color textColor = {255, 255, 255, 255},
                const std::string& fontPath = "assets/font.ttf");
    
    void setPosition(int x, int y);
    void setSize(int w, int h);
    
    void handleEvent(const SDL_Event& e);
    void render();
    
    std::string getText() const { return currentInput; }
    void setText(const std::string& text);
    void clear() { currentInput.clear(); cursorPos = 0; updateDisplayText(); }
    
    void cleanup();
};