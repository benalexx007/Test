#include "start.h"
#include <iostream>
#include <fstream>
#include "ingame/panel.h"

Start::Start() {}
Start::~Start() { cleanup(); }

void Start::init()
{
    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init() == 0;

    // create a new window + renderer (like Game)
    window = SDL_CreateWindow("Mê Cung Tây Du", winW, winH, SDL_WINDOW_RESIZABLE);
    renderer = SDL_CreateRenderer(window, NULL);
    SDL_MaximizeWindow(window);

    // load background image (single image file)
    bgTexture = IMG_LoadTexture(renderer, "assets/images/background/background.png");

    const SDL_Color TextColor = { 0xf9, 0xf2, 0x6a, 0xFF };

    // detect whether users file exists
    bool hasFile = user.read(); // false when file missing
    user.Init();

    auto createMainButtons = [this, &TextColor]() {
        // create PLAY button (centered)
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

        // create SETTINGS button below play
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

    // If users file exists show normal UI; otherwise show single large CREATE AN ACCOUNT button.
    if (hasFile) {
        createMainButtons();
    } else {
        // single large button (700x170, line break after ACCOUNT)
        const int BtnW = 700;
        const int BtnH = 170;
        const int TextSize = 96;
        int xCenter = (winW - BtnW) / 2;
        int yBtn = static_cast<int>(winH * 0.4f);
        playBtn = std::make_unique<Button>(renderer);
        if (!playBtn->create(renderer, xCenter, yBtn, BtnW, BtnH, "CREATE AN\nACCOUNT TO PLAY", TextSize, TextColor, "assets/font.ttf")) {
            std::cerr << "Start::init - failed to create CreateAccount button\n";
        } else {
            playBtn->setLabelPositionPercent(0.5f, 0.55f);
            // when clicked, show AccountPanel; after sign-in, rebuild UI to normal
            playBtn->setCallback([this]() {
                // build AccountPanel and pass callback to refresh Start UI after signin
                accountPanel.reset();
                accountPanel = std::make_unique<AccountPanel>(renderer);
                bool fileNowExists = user.read();
                user.Init();
                accountPanel->init(&user, fileNowExists, winW, winH, [this]() {
                    // after account created / changed: recreate main buttons
                    // cleanup the "create account" button and panel, then show normal UI
                    if (playBtn) { playBtn->cleanup(); playBtn.reset(); }
                    if (settingsBtn) { settingsBtn->cleanup(); settingsBtn.reset(); }
                    if (accountPanel) { accountPanel->cleanup(); accountPanel.reset(); }
                    // re-read user state
                    user.read();
                    user.Init();
                    // create main buttons now
                    const SDL_Color TextColorLocal = { 0xf9, 0xf2, 0x6a, 0xFF };
                    // duplicate createMainButtons body here (cannot capture the local lambda)
                    const int BtnW2 = 350;
                    const int BtnH2 = 85;
                    int xCenter2 = (winW - BtnW2) / 2;
                    int yPlay2 = static_cast<int>(winH * 0.4f);
                    playBtn = std::make_unique<Button>(renderer);
                    if (!playBtn->create(renderer, xCenter2, yPlay2, BtnW2, BtnH2, "PLAY", 72, TextColorLocal, "assets/font.ttf")) {
                        std::cerr << "Start - failed to create Play button after signin\n";
                    } else {
                        playBtn->setLabelPositionPercent(0.5f, 0.70f);
                    }
                    const int Padding2 = 30;
                    int ySettings2 = yPlay2 + BtnH2 + Padding2;
                    settingsBtn = std::make_unique<Button>(renderer);
                    if (!settingsBtn->create(renderer, xCenter2, ySettings2, 350, 85, "SETTINGS", 72, TextColorLocal, "assets/font.ttf")) {
                        std::cerr << "Start - failed to create Settings button after signin\n";
                    } else {
                        settingsBtn->setLabelPositionPercent(0.5f, 0.70f);
                    }
                });
            });
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
        // forward to buttons (they check coords themselves)
        if (playBtn) playBtn->handleEvent(e);
        if (settingsBtn) settingsBtn->handleEvent(e);

        // forward to account panel if visible
        if (accountPanel) accountPanel->handleEvent(e);

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