#include "textbox.h"
#include <cstring>
#include <iostream>
#include <algorithm>

Textbox::Textbox(SDL_Renderer* renderer) : renderer(renderer) {}
Textbox::~Textbox() { cleanup(); }

bool Textbox::create(SDL_Renderer* rend, int x, int y, int w, int h,
                     const std::string& bgPath,
                     const std::string& placeholderText,
                     int fs, SDL_Color tc, const std::string& fp)
{
    cleanup();
    renderer = rend;
    if (!renderer) return false;
    
    rect = {x, y, w, h};
    placeholderStr = placeholderText;
    fontSize = fs;
    textColor = tc;
    fontPath = fp;
    cursorPos = 0;
    
    // load background texture
    bgTexture = IMG_LoadTexture(renderer, bgPath.c_str());
    if (!bgTexture) {
        std::cerr << "Textbox::create - failed to load " << bgPath << " | " << SDL_GetError() << "\n";
        return false;
    }
    
    // create placeholder text
    placeholder = std::make_unique<Text>(renderer);
    if (!placeholder->create(renderer, fontPath, fontSize, placeholderStr, placeholderColor)) {
        std::cerr << "Textbox::create - failed to create placeholder text\n";
        return false;
    }
    
    // create input text (empty initially)
    inputText = std::make_unique<Text>(renderer);
    if (!inputText->create(renderer, fontPath, fontSize, "", textColor)) {
        std::cerr << "Textbox::create - failed to create input text\n";
        return false;
    }
    
    // create cursor text (for measuring cursor position)
    cursorText = std::make_unique<Text>(renderer);
    if (!cursorText->create(renderer, fontPath, fontSize, "|", cursorColor)) {
        std::cerr << "Textbox::create - failed to create cursor text\n";
        return false;
    }
    
    updateDisplayText();
    return true;
}

void Textbox::updateCursorBlink()
{
    Uint32 now = SDL_GetTicks();
    if (now - lastCursorToggle >= CURSOR_BLINK_MS) {
        cursorVisible = !cursorVisible;
        lastCursorToggle = now;
    }
}

void Textbox::updateDisplayText()
{
    if (!inputText || !placeholder) return;
    
    // Placeholder shown only if input is empty AND not focused
    if (currentInput.empty() && !focused) {
        placeholder->setText(placeholderStr);
        placeholder->setPosition(rect.x + 40, rect.y + (rect.h - placeholder->getHeight()) / 2);
    } else if (!currentInput.empty() || focused) {
        // Show input text (with cursor appended if focused and visible)
        std::string displayText = currentInput;
        if (focused && cursorVisible && cursorPos <= currentInput.length()) {
            displayText.insert(cursorPos, "|");
        }
        
        inputText->setText(displayText);
        inputText->setPosition(rect.x + 40, rect.y + (rect.h - inputText->getHeight()) / 2);
    }
}

void Textbox::setPosition(int x, int y)
{
    rect.x = x;
    rect.y = y;
    updateDisplayText();
}

void Textbox::setSize(int w, int h)
{
    rect.w = w;
    rect.h = h;
    updateDisplayText();
}

void Textbox::handleEvent(const SDL_Event& e)
{
    if (!renderer) return;
    
    // check if clicked (mouse down)
    if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
        int mx = e.button.x;
        int my = e.button.y;
        bool inside = (mx >= rect.x && mx < rect.x + rect.w && my >= rect.y && my < rect.y + rect.h);
        if (inside && !focused) {
            focused = true;
            cursorPos = currentInput.length();  // place cursor at end on initial focus
            lastCursorToggle = SDL_GetTicks();
            cursorVisible = true;
            SDL_StartTextInput(SDL_GetKeyboardFocus());
            updateDisplayText();
        } else if (!inside && focused) {
            focused = false;
            SDL_StopTextInput(SDL_GetKeyboardFocus());
            updateDisplayText();
        }
    }
    
    // handle text input (insert at cursor position)
    if (focused && e.type == SDL_EVENT_TEXT_INPUT) {
        const char* text = e.text.text;
        if (text && text[0] != '\0') {
            const size_t MAX_INPUT_LEN = 4096; // keep input bounded
            size_t addLen = std::strlen(text);
            if (currentInput.size() + addLen > MAX_INPUT_LEN) {
                // truncate appended text to fit
                size_t canAdd = (currentInput.size() < MAX_INPUT_LEN) ? (MAX_INPUT_LEN - currentInput.size()) : 0;
                if (canAdd > 0) currentInput.insert(cursorPos, std::string(text, text + canAdd));
                cursorPos = std::min(cursorPos + canAdd, currentInput.length());
            } else {
                currentInput.insert(cursorPos, text);
                cursorPos += addLen;
            }
            updateDisplayText();
        }
    }
    
    // handle arrow keys and backspace/delete
    if (focused && e.type == SDL_EVENT_KEY_DOWN) {
        switch (e.key.key) {
            case SDLK_LEFT:
                // move cursor left
                if (cursorPos > 0) {
                    cursorPos--;
                    cursorVisible = true;
                    lastCursorToggle = SDL_GetTicks();
                    updateDisplayText();
                }
                break;
            case SDLK_RIGHT:
                // move cursor right
                if (cursorPos < currentInput.length()) {
                    cursorPos++;
                    cursorVisible = true;
                    lastCursorToggle = SDL_GetTicks();
                    updateDisplayText();
                }
                break;
            case SDLK_HOME:
                // move cursor to start
                cursorPos = 0;
                cursorVisible = true;
                lastCursorToggle = SDL_GetTicks();
                updateDisplayText();
                break;
            case SDLK_END:
                // move cursor to end
                cursorPos = currentInput.length();
                cursorVisible = true;
                lastCursorToggle = SDL_GetTicks();
                updateDisplayText();
                break;
            case SDLK_BACKSPACE:
                // delete character before cursor
                if (cursorPos > 0) {
                    currentInput.erase(cursorPos - 1, 1);
                    cursorPos--;
                    updateDisplayText();
                }
                break;
            case SDLK_DELETE:
                // delete character at cursor
                if (cursorPos < currentInput.length()) {
                    currentInput.erase(cursorPos, 1);
                    updateDisplayText();
                }
                break;
            default:
                break;
        }
    }
}

void Textbox::render()
{
    if (!renderer) return;
    
    // update cursor blink state
    if (focused) {
        updateCursorBlink();
        updateDisplayText();
    }
    
    // draw background
    if (bgTexture) {
        SDL_FRect dst = {(float)rect.x, (float)rect.y, (float)rect.w, (float)rect.h};
        SDL_RenderTexture(renderer, bgTexture, nullptr, &dst);
    }
    
    // draw text (placeholder if empty and unfocused, otherwise input with cursor)
    if (currentInput.empty() && !focused) {
        if (placeholder) placeholder->render();
    } else {
        if (inputText) inputText->render();
    }
}

void Textbox::setText(const std::string& text)
{
    currentInput = text;
    cursorPos = text.length();
    updateDisplayText();
}

void Textbox::cleanup()
{
    if (focused) {
        SDL_StopTextInput(SDL_GetKeyboardFocus());
        focused = false;
    }

    if (bgTexture) {
        SDL_DestroyTexture(bgTexture);
        bgTexture = nullptr;
    }
    if (placeholder) {
        placeholder->cleanup();
        placeholder.reset();
    }
    if (inputText) {
        inputText->cleanup();
        inputText.reset();
    }
    if (cursorText) {
        cursorText->cleanup();
        cursorText.reset();
    }
}