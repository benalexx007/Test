#pragma once
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <string>
#include "../map/map.h"

class Character {
protected:
    SDL_Renderer* renderer;
    SDL_Texture* texture;
    int x, y;
    int tileSize;

    float fx, fy; // vị trí thực tế cho animation mượt

public:
    Character(SDL_Renderer* renderer, const std::string& path, int startX, int startY, int tileSize);
    virtual ~Character();
    virtual void render(int offsetX = 0, int offsetY = 0);
    bool canMoveTo(Map* map, int nx, int ny);
    void moveTo(int nx, int ny);
    bool isAtRest() const;
    void updatePosition(float speed = 0.2f); // gọi mỗi frame để tween

    int getX() const { return x; }
    int getY() const { return y; }
    
};