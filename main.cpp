#include <SDL3/SDL.h>
#include <SDL3/SDL_image.h>
#include <iostream>
#include "map.cpp"  // 👈 chỉ cần include file map.cpp

class Game {
private:
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    bool isRunning = false;

public:
    Map* map = nullptr;

    void init() {
        SDL_Init(SDL_INIT_VIDEO);
        window = SDL_CreateWindow("Mummy Maze Demo (SDL3)", 640, 640, 0);
        renderer = SDL_CreateRenderer(window, NULL);

        map = new Map(renderer);
        map->loadFromFile("assets/maps/tutorial.txt");

        isRunning = true;
    }

    void gameLoop() {
        while (isRunning) {
            handleEvents();
            render();
        }
    }

    void handleEvents() {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT)
                isRunning = false;
        }
    }

    void render() {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        map->render();

        SDL_RenderPresent(renderer);
    }

    void cleanup() {
        delete map;
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
    }
};

int main(int argc, char* argv[]) {
    Game game;
    game.init();
    if (game.map) game.gameLoop();
    game.cleanup();

    std::cout << "Press Enter to exit...";
    std::cin.get();
    return 0;
}
