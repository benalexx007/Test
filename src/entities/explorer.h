#pragma once
#include "character.h"

class Explorer : public Character {
private:
    bool moved = false; // báo có di chuyển trong lượt

public:
    Explorer(SDL_Renderer* renderer, int startX, int startY, int tileSize, const std::string& stage);
    void handleInput(const SDL_Event& e, Map* map);
    bool hasMoved() { return moved; }
    void resetMoveFlag() { moved = false; }
};