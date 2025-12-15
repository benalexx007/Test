#include "game.h"
#include <cmath>

void Game::init(char stage)
{
    SDL_Init(SDL_INIT_VIDEO);

    // initialize SDL_ttf before any Text/TTF usage
    if (TTF_Init() == 0) {
        std::cerr << "TTF_Init failed: " << SDL_GetError() << "\n";
    }

    window = SDL_CreateWindow("Mê Cung Tây Du", winW, winH, SDL_WINDOW_RESIZABLE);
    renderer = SDL_CreateRenderer(window, NULL);
    SDL_MaximizeWindow(window);

    // background manager
    background = new Background(renderer);
    background->load(stage);

    map = new Map(renderer, stage);
    map->loadFromFile("assets/maps/level" + std::string(1, stage) + ".txt");
    int tileSize = map->getTileSize();
    int mapPxW = tileSize * map->getCols();
    int mapPxH = tileSize * map->getRows();
    offsetX = (winW - mapPxW) * 95 / 100;
    offsetY = (winH - mapPxH) / 2;

    ingamePanel = new IngamePanel(renderer);
    ingamePanel->create(renderer, 0, 0, 0, 0);
    ingamePanel->initForStage(this, winW, mapPxW, winH, mapPxH);

    explorer = new Explorer(renderer, 1, 1, tileSize, stage);
    mummy = new Mummy(renderer, 5, 5, tileSize, stage);

    isRunning = true;
}

void Game::handleEvents()
{
    SDL_Event e;
    int curW = winW;
    int curH = winH;
    while (SDL_PollEvent(&e))
    {
        if (e.type == SDL_EVENT_QUIT)
            isRunning = false;

        // forward input to UI panels so buttons get clicks
        if (ingamePanel) ingamePanel->handleEvent(e);
        if (settingsPanel) settingsPanel->handleEvent(e);

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

        if (turn == 0)
            explorer->handleInput(e, map);
    }
}

void Game::update()
{
    // tween vị trí mỗi frame
    explorer->updatePosition();
    mummy->updatePosition();

    // Nếu đang là lượt người chơi và người chơi vừa đi xong → bắt đầu lượt mummy (2 bước)
    if (turn == 0 && explorer->hasMoved())
    {
        turn = 1;
        mummyStepsLeft = 2;
        explorer->resetMoveFlag();
    }

    // Lượt của mummy: đi từng bước khi đã đứng tâm ô
    if (turn == 1)
    {
        if (mummyStepsLeft > 0 && mummy->isAtRest())
        {
            mummy->moveOneStep(map, explorer->getX(), explorer->getY());
            mummyStepsLeft--;
        }
        // Khi đã đi đủ 2 bước và dừng lại, trả lượt về cho người chơi
        if (mummyStepsLeft == 0 && mummy->isAtRest())
        {
            turn = 0;
        }
    }
}

void Game::render()
{
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    if (background) background->render(winW, winH);
    map->render(offsetX, offsetY);
    explorer->render(offsetX, offsetY);
    mummy->render(offsetX, offsetY);
    if (ingamePanel) ingamePanel->render();

    SDL_RenderPresent(renderer);
}

void Game::cleanup()
{
    if (background) {
        background->cleanup();
        delete background;
        background = nullptr;
    }

    if (ingamePanel) {
        ingamePanel->cleanup();
        delete ingamePanel;
        ingamePanel = nullptr;
    }
    
    delete map;
    map = nullptr;

    delete explorer;
    explorer = nullptr;

    delete mummy;
    mummy = nullptr;

    SDL_DestroyRenderer(renderer);
    renderer = nullptr;

    SDL_DestroyWindow(window);
    window = nullptr;

    SDL_Quit();
    TTF_Quit();

    isRunning = false;
}

void Game::run(char stage)
{
    init(stage);
    while (isRunning)
    {
        handleEvents();
        update();
        render();
        SDL_Delay(16); // ~60 FPS
    }
    cleanup();
}