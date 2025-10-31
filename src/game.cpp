#include "game.h"

void Game::init() {
    SDL_Init(SDL_INIT_VIDEO);

    window = SDL_CreateWindow("Mummy Maze (SDL3)", 640, 640, 0);
    renderer = SDL_CreateRenderer(window, NULL);

    map = new Map(renderer);
    map->loadFromFile("assets/maps/tutorial.txt");

    int tileSize = map->getTileSize();
    explorer = new Explorer(renderer, 1, 1, tileSize);
    mummy = new Mummy(renderer, 5, 5, tileSize);

    isRunning = true;
}

void Game::handleEvents() {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_EVENT_QUIT)
            isRunning = false;

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

    map->render();
    explorer->render();
    mummy->render();

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

void Game::run() {
    init();
    while (isRunning) {
        handleEvents();
        update();
        render();
        SDL_Delay(16); // ~60 FPS
    }
    cleanup();
}