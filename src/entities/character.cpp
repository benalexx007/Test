#include "character.h"
#include <iostream>
#include <cmath>

Character::Character(SDL_Renderer* renderer, const std::string& path, int startX, int startY, int tileSize)
    : renderer(renderer), x(startX), y(startY), tileSize(tileSize), fx((float)startX), fy((float)startY)
{
    texture = IMG_LoadTexture(renderer, path.c_str());
    if (!texture)
        std::cerr << "Failed to load texture: " << path << " | " << SDL_GetError() << std::endl;
}

Character::~Character() { SDL_DestroyTexture(texture); }

void Character::render() {
    SDL_FRect rect = { fx * tileSize, fy * tileSize, (float)tileSize, (float)tileSize };
    SDL_RenderTexture(renderer, texture, NULL, &rect);
}

bool Character::canMoveTo(Map* map, int nx, int ny) {
    return !map->isWall(nx, ny);
}
bool Character::isAtRest() const {
    return std::fabs(fx - x) < 0.01f && std::fabs(fy - y) < 0.01f;
}
void Character::moveTo(int nx, int ny) {
    x = nx; y = ny;
}

void Character::updatePosition(float speed) {
    fx += (x - fx) * speed;
    fy += (y - fy) * speed;
    if (std::fabs(fx - x) < 0.01f) fx = (float)x;
    if (std::fabs(fy - y) < 0.01f) fy = (float)y;
}