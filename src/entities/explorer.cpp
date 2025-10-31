#include "explorer.h"

Explorer::Explorer(SDL_Renderer* renderer, int startX, int startY, int tileSize)
    : Character(renderer, "assets/images/explorer.png", startX, startY, tileSize) {}

void Explorer::handleInput(const SDL_Event& e, Map* map) {
    if (e.type != SDL_EVENT_KEY_DOWN) return;

    // Chỉ cho phép nhận input khi đã đứng giữa ô (tránh "bay")
    if (fx != x || fy != y) return;

    int nx = x, ny = y;

    switch (e.key.key) {
        case SDLK_UP:    ny--; break;
        case SDLK_DOWN:  ny++; break;
        case SDLK_LEFT:  nx--; break;
        case SDLK_RIGHT: nx++; break;
        default: return;
    }

    if (canMoveTo(map, nx, ny)) {
        moveTo(nx, ny);
        moved = true;
    }
}