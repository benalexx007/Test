#pragma once
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <memory>
#include "ingame/button.h"
#include "ingame/background.h"
#include "ingame/panel.h"
#include "user.h"

class Start {
public:
    Start();
    ~Start();

    // create window and UI; returns false on failure
    void init();

    // main loop for the start window (blocking)
    void run();

    // cleanup resources
    void cleanup();

private:
    void handleEvents();
    void render();
    void createMainButtons();

    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_Texture* bgTexture = nullptr;

    std::unique_ptr<Button> playBtn;
    std::unique_ptr<Button> settingsBtn;

    // account UI + user storage
    User user; // default filepath "users.bin"
    std::unique_ptr<AccountPanel> accountPanel;
    bool pendingShowMainButtons = false;
    std::unique_ptr<SettingsPanel> settingsPanel;
    bool settingsVisible = false;
    bool isRunning = false;
    int winW = 1920;
    int winH = 991;
    float windowRatio = 1920.0/991.0;
};