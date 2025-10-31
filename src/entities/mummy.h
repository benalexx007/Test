#pragma once
#include "character.h"

class Mummy : public Character {
public:
    Mummy(SDL_Renderer* renderer, int startX, int startY, int tileSize);

    void moveOneStep(Map* map, int targetX, int targetY);
    void chase(Map* map, int targetX, int targetY);
};