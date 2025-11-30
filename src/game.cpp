#include "game.h"
#include <cmath>

void Game::init(const std::string& stage) {
    SDL_Init(SDL_INIT_VIDEO);

    window = SDL_CreateWindow("Mummy Maze (SDL3)", winW, winH, SDL_WINDOW_RESIZABLE);
    renderer = SDL_CreateRenderer(window, NULL);
    SDL_MaximizeWindow(window);

    map = new Map(renderer, stage);
    map->loadFromFile("assets/maps/level"+stage+".txt");

    int mapPxW = map->getTileSize() * map->getCols();
    int mapPxH = map->getTileSize() * map->getRows();
    offsetX = (winW - mapPxW)*95/100;
    offsetY = (winH - mapPxH) / 2;

    int tileSize = map->getTileSize();
    explorer = new Explorer(renderer, 1, 1, tileSize, stage);
    mummy = new Mummy(renderer, 5, 5, tileSize, stage);

    isRunning = true;
}

void Game::handleEvents() {
    SDL_Event e;
    int curW = winW;
    int curH = winH;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_EVENT_QUIT)
            isRunning = false;
        
        if (e.type == SDL_EVENT_WINDOW_RESIZED) {
            int w = e.window.data1;
            int h = e.window.data2;
            if (w != curW){
                h = static_cast<int>(w / windowRatio);
            }
            else if (h != curH){
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

void Game::update() {
    // tween vị trí mỗi frame
    explorer->updatePosition();
    mummy->updatePosition();

    // Nếu đang là lượt người chơi và người chơi vừa đi xong → bắt đầu lượt mummy (2 bước)
    if (turn == 0 && explorer->hasMoved()) {
        turn = 1;
        mummyStepsLeft = 2;
        explorer->resetMoveFlag();
    }

    // Lượt của mummy: đi từng bước khi đã đứng tâm ô
    if (turn == 1) {
        if (mummyStepsLeft > 0 && mummy->isAtRest()) {
            mummy->moveOneStep(map, explorer->getX(), explorer->getY());
            mummyStepsLeft--;
        }
        // Khi đã đi đủ 2 bước và dừng lại, trả lượt về cho người chơi
        if (mummyStepsLeft == 0 && mummy->isAtRest()) {
            turn = 0;
        }
    }
}

void Game::render() {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    map->render(offsetX, offsetY);
    explorer->render(offsetX, offsetY);
    mummy->render(offsetX, offsetY);

    SDL_RenderPresent(renderer);
}

void Game::cleanup() {
    delete map;
    delete explorer;
    delete mummy;
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

void Game::run(const std::string& stage) {
    init(stage);
    while (isRunning) {
        handleEvents();
        update();
        render();
        SDL_Delay(16); // ~60 FPS
    }
    cleanup();
}