#pragma once
#include "character.h"

class IMummyAI;
class Map;

class Mummy : public Character {
private:
    IMummyAI* ai = nullptr;

public:
    Mummy(SDL_Renderer* renderer, int startX, int startY, int tileSize, char stage);
    ~Mummy();

    void moveOneStep(Map* map, int targetX, int targetY);
    void chase(Map* map, int targetX, int targetY);
    void setAI(IMummyAI* aiInstance);
    IMummyAI* getAI() const { return ai; }
};