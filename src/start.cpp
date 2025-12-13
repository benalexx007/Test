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

    // createMainButtons moved to member function
    (void)TextColor; // silence unused warning until member method uses it

    if (hasFile) {
        createMainButtons();
    } else {
        // No user file: show account creation panel
        accountPanel.reset();
        accountPanel = std::make_unique<AccountPanel>(renderer);
        const int panelW = 1750;
        const int panelH = 900;
        if (accountPanel->init(&user, false, panelW, panelH, [this]() {
            // Request showing main buttons — defer actual cleanup until
            // we're out of event handling to avoid deleting the panel
            // while it's processing its own event.
            pendingShowMainButtons = true;
            // refresh user state now (safe)
            user.read();
            user.Init();
        })) {
            int px = (winW - panelW) / 2;
            int py = (winH - panelH) / 2;
            accountPanel->setPosition(px, py);
        }
    }

    isRunning = true;
    SDL_MaximizeWindow(window);
}

void Start::createMainButtons()
{
    const SDL_Color TextColor = { 0xf9, 0xf2, 0x6a, 0xFF };
    const int BtnW = 350;
    const int BtnH = 85;
    int xCenter = (winW - BtnW) / 2;
    int yPlay = static_cast<int>(winH * 0.4f);
    playBtn = std::make_unique<Button>(renderer);
    if (!playBtn->create(renderer, xCenter, yPlay, BtnW, BtnH, "PLAY", 72, TextColor, "assets/font.ttf")) {
        std::cerr << "Start::createMainButtons - failed to create Play button\n";
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
        std::cerr << "Start::createMainButtons - failed to create Settings button\n";
    } else {
        settingsBtn->setLabelPositionPercent(0.5f, 0.70f);
        settingsBtn->setCallback([this]() {
            // toggle settings panel
            if (settingsVisible) {
                settingsVisible = false;
                if (settingsPanel) { settingsPanel->cleanup(); settingsPanel.reset(); }
                return;
            }
            settingsPanel = std::make_unique<SettingsPanel>(renderer);
            const int panelW = 1750, panelH = 900;
            // Truyền callback để đóng panel từ bên trong SettingsPanel
            if (!settingsPanel->init(&user, panelW, panelH, [this]() {
                // Callback này sẽ được gọi khi nhấn nút QUIT trong SettingsPanel
                settingsVisible = false;
                if (settingsPanel) { settingsPanel->cleanup(); settingsPanel.reset(); }
            })) {
                settingsPanel.reset();
                return;
            }
            int px = (winW - panelW) / 2;
            int py = (winH - panelH) / 2;
            settingsPanel->setPosition(px, py);
            settingsVisible = true;
        });
    }
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
        if (!panelActive && settingsVisible && settingsPanel) {
            settingsPanel->handleEvent(e);
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

    // If account panel requested to be closed/show main buttons, perform cleanup now
    if (pendingShowMainButtons) {
        if (accountPanel) { accountPanel->cleanup(); accountPanel.reset(); }
        // user state was refreshed when request created; recreate main buttons
        createMainButtons();
        pendingShowMainButtons = false;
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
    if (accountPanel) accountPanel->render();
    if (settingsVisible && settingsPanel) settingsPanel->render();

    SDL_RenderPresent(renderer);
}

void Start::cleanup()
{
    if (playBtn) { playBtn->cleanup(); playBtn.reset(); }
    if (settingsBtn) { settingsBtn->cleanup(); settingsBtn.reset(); }

    if (accountPanel) { accountPanel->cleanup(); accountPanel.reset(); }
    if (settingsVisible && settingsPanel) { settingsPanel->cleanup(); settingsPanel.reset(); }
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