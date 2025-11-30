#pragma once
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <iostream>
#include "map.h"
#include "entities/explorer.h"
#include "entities/mummy.h"

class Game {
private:
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    bool isRunning = false;
    int turn = 0; // 0 = Explorer, 1 = Mummy
    int mummyStepsLeft = 0;
    int winW = 1920;
    int winH = 991;
    float windowRatio = 1920.0/991.0;
    int offsetX = 0;
    int offsetY = 0;
public:
    Map* map = nullptr;
    Explorer* explorer = nullptr;
    Mummy* mummy = nullptr;

    void init(const std::string& stage);
    void handleEvents();
    void update();
    void recalcLayout(int winW, int winH);
    void render();
    void cleanup();
    void run(const std::string& stage);
};