#include "start.h"
#include <iostream>
#include <fstream>
#include "ingame/panel.h"
#include "stages.h"
#include "game.h"

Start::Start() {}
Start::~Start() { cleanup(); }

void Start::init()
{
    renderer = SDL_CreateRenderer(window, NULL);
    bgTexture = IMG_LoadTexture(renderer, "assets/images/background/background.png");

    const SDL_Color TextColor = { 0xf9, 0xf2, 0x6a, 0xFF };

    bool hasFile = user.read();
    user.Init();

    // Kiểm tra xem user đã đăng nhập hay chưa
    bool isLoggedIn = user.isLoggedIn();

    // createMainButtons moved to member function
    (void)TextColor; // silence unused warning until member method uses it

    // Nếu không có file -> hiện màn hình đăng ký
    if (!hasFile) {
        accountPanel.reset();
        accountPanel = std::make_unique<AccountPanel>(renderer);
        const int panelW = 1750;
        const int panelH = 900;
        if (accountPanel->init(&user, false, panelW, panelH, [this]() {
            pendingShowMainButtons = true;
            user.read();
            user.Init();
        })) {
            int px = (winW - panelW) / 2;
            int py = (winH - panelH) / 2;
            accountPanel->setPosition(px, py);
        }
    }
    // Nếu có file nhưng chưa đăng nhập -> hiện màn hình SIGN IN / LOG IN
    else if (!isLoggedIn) {
        accountPanel.reset();
        accountPanel = std::make_unique<AccountPanel>(renderer);
        const int panelW = 1750;
        const int panelH = 900;
        if (accountPanel->init(&user, true, panelW, panelH, [this]() {
            // Sau khi đăng nhập thành công, hiện main buttons
            pendingShowMainButtons = true;
            user.read();
            user.Init();
        })) {
            int px = (winW - panelW) / 2;
            int py = (winH - panelH) / 2;
            accountPanel->setPosition(px, py);
        }
    }
    // Nếu đã đăng nhập -> hiện main buttons
    else {
        createMainButtons();
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
            // start sliding buttons out to the left and then open stages view
            if (buttonsSlidingOut) return;   
            std::cerr << "Start: PLAY clicked, starting slide\n";
            // record start positions
            playBtnStartX = playBtn ? playBtn->getX() : 0;
            settingsBtnStartX = settingsBtn ? settingsBtn->getX() : 0;
            slideStartTime = SDL_GetTicks();
            buttonsSlidingOut = true;
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
        if (!panelActive && accountPanel) {
            accountPanel->handleEvent(e);
            panelActive = true;
        }
        if (!panelActive && settingsVisible && settingsPanel) {
            settingsPanel->handleEvent(e);
            panelActive = true;
        }

        // if stages view present, let it handle input
        if (!panelActive && stagesView) {
            stagesView->handleEvent(e);
            continue;
        }

        if (!panelActive && !stagesView) {
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

    // animate sliding out buttons when requested
    if (buttonsSlidingOut) {
        Uint32 now = SDL_GetTicks();
        Uint32 elapsed = now - slideStartTime;
        float t = elapsed >= slideDurationMs ? 1.0f : (float)elapsed / (float)slideDurationMs;
        // ease-in-out
        t = 1.0f - std::pow(1.0f - t, 3);
        int targetOffset = winW + 200; // move far to left (negative)
        if (playBtn) {
            int sx = playBtnStartX;
            int nx = static_cast<int>(sx - t * (sx + targetOffset));
            playBtn->setPosition(nx, playBtn->getY());
        }
        if (settingsBtn) {
            int sx = settingsBtnStartX;
            int nx = static_cast<int>(sx - t * (sx + targetOffset));
            settingsBtn->setPosition(nx, settingsBtn->getY());
        }
        if (t >= 1.0f) {
            // finished sliding: destroy buttons and open stages view
            if (playBtn) { playBtn->cleanup(); playBtn.reset(); }
            if (settingsBtn) { settingsBtn->cleanup(); settingsBtn.reset(); }
            buttonsSlidingOut = false;

            std::cerr << "Start::render - slide finished, creating Stages view\n";
            stagesView = std::make_unique<Stages>(renderer);
            bool ok = stagesView->init(renderer, &user, winW, winH, [this](char stageChar) {
                // start game with selected stage char: cleanup UI first
                this->cleanup(); // destroys window/renderer and quits SDL subsystems
                Game game;
                game.run(stageChar, window);
                isRunning = false;
            });
            std::cerr << "Start::render - Stages::init returned=" << (ok ? "true" : "false") << "\n";
            if (!ok) stagesView.reset();
        }
    }

    // draw buttons (if still present)
    if (playBtn) playBtn->render();
    if (settingsBtn) settingsBtn->render();

    // render stages view over background if present
    if (stagesView) stagesView->render();

    // render panels over other UI if present
    if (settingsVisible && settingsPanel) settingsPanel->render();
    if (accountPanel) accountPanel->render();

    SDL_RenderPresent(renderer);
}

void Start::cleanup()
{
    if (playBtn) { playBtn->cleanup(); playBtn.reset(); }
    if (settingsBtn) { settingsBtn->cleanup(); settingsBtn.reset(); }
    if (settingsVisible && settingsPanel) { settingsPanel->cleanup(); settingsPanel.reset(); }
    if (accountPanel) { accountPanel->cleanup(); accountPanel.reset(); }

    if (bgTexture) { SDL_DestroyTexture(bgTexture); bgTexture = nullptr; }
    
    SDL_DestroyRenderer(renderer); renderer = nullptr;

    isRunning = false;
}

void Start::run(SDL_Window *win)
{
    window = win;
    init();
    while (isRunning) {
        handleEvents();
        render();
        SDL_Delay(16); // ~60FPS
    }
    cleanup();
}