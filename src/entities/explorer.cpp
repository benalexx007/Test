#include "explorer.h"
#include "explorer.h"

Explorer::Explorer(SDL_Renderer* renderer, int startX, int startY, int tileSize, char stage)
    : Character(renderer, "assets/images/explorer/explorer", std::string(1, stage), startX, startY, tileSize) {}

// Process keyboard input for tile-based movement. Input is only
// considered when the explorer is at rest (no ongoing interpolation)
// to prevent queued inputs from producing unrealistic movement.
void Explorer::handleInput(const SDL_Event& e, Map* map) {
    if (e.type != SDL_EVENT_KEY_DOWN) return;

    // Only accept input when the visual position equals the logical
    // tile coordinates (i.e., the character is not currently moving).
    if (fx != x || fy != y) return;

    int nx = x, ny = y;

    switch (e.key.key) {
        case SDLK_UP:    ny--; break;
        case SDLK_DOWN:  ny++; break;
        case SDLK_LEFT:  nx--; break;
        case SDLK_RIGHT: nx++; break;
        default: return;
    }

    // If the target tile is walkable, move the explorer. If the
    // target is a wall, this implementation still counts the
    // attempted action as a move; the intended gameplay effect is
    // that a blocked attempt consumes the player's turn.
    if (canMoveTo(map, nx, ny)) {
        moveTo(nx, ny);
        moved = true;
    } else {
        moved = true; // blocked attempt counts as a move
    }
}