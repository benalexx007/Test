#include "start.h"
#include <iostream>
#include <fstream>
#include "ingame/panel.h"

Start::Start() {}
Start::~Start() { cleanup(); }

void Start::init()
{
    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();

    window = SDL_CreateWindow("Mê Cung Tây Du", winW, winH, SDL_WINDOW_RESIZABLE);
    renderer = SDL_CreateRenderer(window, NULL);
    SDL_MaximizeWindow(window);

    bgTexture = IMG_LoadTexture(renderer, "assets/images/background/background.png");

    const SDL_Color TextColor = { 0xf9, 0xf2, 0x6a, 0xFF };

    bool hasFile = user.read();
    user.Init();

    auto createMainButtons = [this, &TextColor]() {
        const int BtnW = 350;
        const int BtnH = 85;
        int xCenter = (winW - BtnW) / 2;
        int yPlay = static_cast<int>(winH * 0.4f);
        playBtn = std::make_unique<Button>(renderer);
        if (!playBtn->create(renderer, xCenter, yPlay, BtnW, BtnH, "PLAY", 72, TextColor, "assets/font.ttf")) {
            std::cerr << "Start::init - failed to create Play button\n";
        } else {
            playBtn->setLabelPositionPercent(0.5f, 0.70f);
            playBtn->setCallback([this]() {
                // TODO: start the game
            });
        }

        const int Padding = 30;
        int ySettings = yPlay + BtnH + Padding;
        settingsBtn = std::make_unique<Button>(renderer);
        if (!settingsBtn->create(renderer, xCenter, ySettings, 350, 85, "SETTINGS", 72, TextColor, "assets/font.ttf")) {
            std::cerr << "Start::init - failed to create Settings button\n";
        } else {
            settingsBtn->setLabelPositionPercent(0.5f, 0.70f);
            settingsBtn->setCallback([]() {
                // TODO: open settings window
            });
        }
    };

    if (hasFile) {
        createMainButtons();
    } else {
        // No user file: show account creation panel
        accountPanel.reset();
        accountPanel = std::make_unique<AccountPanel>(renderer);
        const int panelW = 1750;
        const int panelH = 900;
        if (accountPanel->init(&user, false, panelW, panelH, [this, &createMainButtons]() {
            // After account created: show main buttons
            if (accountPanel) { accountPanel->cleanup(); accountPanel.reset(); }
            user.read();
            user.Init();
            createMainButtons();
        })) {
            int px = (winW - panelW) / 2;
            int py = (winH - panelH) / 2;
            accountPanel->setPosition(px, py);
        }
    }

    isRunning = true;
    SDL_MaximizeWindow(window);
}

void Start::handleEvents()
{
    SDL_Event e;
    int curW = winW;
    int curH = winH;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_EVENT_QUIT) {
            isRunning = false;
        }
        // forward to account panel first (if visible, it consumes events inside the panel)
        bool panelActive = false;
        if (accountPanel) {
            accountPanel->handleEvent(e);
            panelActive = true;
        }

        if (!panelActive) {
            if (playBtn) playBtn->handleEvent(e);
            if (settingsBtn) settingsBtn->handleEvent(e);
        }

        // handle window resize by scaling renderer (keep aspect behavior simple)
        if (e.type == SDL_EVENT_WINDOW_RESIZED)
        {
            int w = e.window.data1;
            int h = e.window.data2;
            if (w != curW)
            {
                h = static_cast<int>(w / windowRatio);
            }
            else if (h != curH)
            {
                w = static_cast<int>(h * windowRatio);
            }
            SDL_SetWindowSize(window, w, h);
            float scale = static_cast<float>(w) / static_cast<float>(winW);
            SDL_SetRenderScale(renderer, scale, scale);
            curW = w;
            curH = h;
        }
    }
}

void Start::render()
{
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderClear(renderer);

    // draw background (stretched)
    if (bgTexture) {
        SDL_FRect dst = { 0.0f, 0.0f, static_cast<float>(winW), static_cast<float>(winH) };
        SDL_RenderTexture(renderer, bgTexture, NULL, &dst);
    }

    // draw buttons
    if (playBtn) playBtn->render();
    if (settingsBtn) settingsBtn->render();

    // render account panel over other UI if present
    if (accountPanel) accountPanel->render();

    SDL_RenderPresent(renderer);
}

void Start::cleanup()
{
    if (playBtn) { playBtn->cleanup(); playBtn.reset(); }
    if (settingsBtn) { settingsBtn->cleanup(); settingsBtn.reset(); }

    if (accountPanel) { accountPanel->cleanup(); accountPanel.reset(); }

    if (bgTexture) { SDL_DestroyTexture(bgTexture); bgTexture = nullptr; }


    SDL_DestroyRenderer(renderer); renderer = nullptr;
    SDL_DestroyWindow(window); window = nullptr;

    SDL_Quit();
    TTF_Quit();

    isRunning = false;
}

void Start::run()
{
    init();
    while (isRunning) {
        handleEvents();
        render();
        SDL_Delay(16); // ~60FPS
    }
    cleanup();
}