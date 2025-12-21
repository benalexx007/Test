// Explorer is the player-controlled character. Input handling and
// movement semantics (including how blocked moves are treated) are
// implemented here.
#pragma once
#include "character.h"

class Explorer : public Character {
private:
    // Flag indicating whether the explorer has performed a move
    // during the current turn. This is used by the game loop to
    // transfer control to the mummy AI after the explorer moves.
    bool moved = false;

public:
    Explorer(SDL_Renderer* renderer, int startX, int startY, int tileSize, char stage);

    // Handle keyboard input. Movement keys produce tile-based moves.
    // Notably, an attempted move into a wall is treated as a move
    // attempt (the explorer remains in place) but still consumes the
    // player's turn; this design ensures that blocked attempts do not
    // allow the player to stall indefinitely.
    void handleInput(const SDL_Event& e, Map* map);
    bool hasMoved() { return moved; }
    void resetMoveFlag() { moved = false; }
};