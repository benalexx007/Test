#pragma once
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <functional>
#include <string>
#include "text.h"
#include "user.h"

class User; // forward

class Stages {
public:
    // onSelect(stageString) called when user clicks centered stage and it's selectable
    Stages(SDL_Renderer* renderer = nullptr);
    ~Stages();

    // callback receives selected stage as a single char, e.g. '1','2','3'
    bool init(SDL_Renderer* renderer, User* user, int winW, int winH, std::function<void(char)> onSelect);
    void handleEvent(const SDL_Event& e);
    void update();   // progress animations (called from render loop)
    void render();

private:
    SDL_Renderer* renderer = nullptr;
    User* user = nullptr;
    std::function<void(char)> onSelect;

    SDL_Texture* tex[3] = { nullptr, nullptr, nullptr };
    int winW = 1920, winH = 991;

    // sizes
    const int thumbW = 315;
    const int thumbH = 162;
    const int centerW = 350;
    const int centerH = 180;
    const int gap = 80; // gap between centers of thumbs

    int selected = 0;      // 0..2
    int prevSelected = 0;
    float slideAnim = 0.0f; // 0..1 when animating between selected states
    bool sliding = false;
    Uint32 slideStart = 0;
    Uint32 slideDur = 300; // ms

    // click highlight
    int clickedIndex = -1;
    Uint32 clickHighlightStart = 0;
    Uint32 clickHighlightDuration = 200; // ms

    // question mark text
    Text qmarkText{nullptr};

    // helper
    SDL_FRect computeDstForIndex(int idx, float extraShift = 0.0f) const;
    bool isIndexUnlocked(int idx) const;
    void startSlideTo(int newIndex);
};