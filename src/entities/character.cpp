#include "character.h"
#include <iostream>
#include <cmath>
#include "character.h"
#include <iostream>
#include <cmath>

// Construct the character by loading a sprite texture for the
// specified stage. The `baseName` and `stage` are concatenated to
// form the final file path (e.g., baseName + stage + ".png"). The
// floating point fields `fx` and `fy` are initialized to the logical
// tile coordinates to start with a rest state.
Character::Character(SDL_Renderer *renderer, const std::string &baseName, const std::string &stage, int startX, int startY, int tileSize)
    : renderer(renderer), x(startX), y(startY), tileSize(tileSize), fx((float)startX), fy((float)startY)
{
    std::string path = baseName + stage + ".png";
    texture = IMG_LoadTexture(renderer, path.c_str());
    if (!texture)
        std::cerr << "Failed to load texture: " << path << " | " << SDL_GetError() << std::endl;
}

Character::~Character() { SDL_DestroyTexture(texture); }

// Render the character at its interpolated pixel position. Note the
// use of a vertical offset (fy - 1/4 tile) to anchor the sprite so
// that the character's feet align with the tile grid. The sprite
// height is rendered as 1.25 tiles to allow for character overlap and
// a slightly taller appearance.
void Character::render(int offsetX, int offsetY)
{
    SDL_FRect rect = {
        fx * static_cast<float>(tileSize) + static_cast<float>(offsetX),
        (fy - 1.0f / 4.0f) * static_cast<float>(tileSize) + static_cast<float>(offsetY),
        static_cast<float>(tileSize),
        static_cast<float>(tileSize) * 5.0f / 4.0f
    };
    SDL_RenderTexture(renderer, texture, NULL, &rect);
}

// Collision check against the map: the character can move to a tile
// if that tile is not a wall. The semantics of `isWall` are defined by
// the Map class and typically check map data for blocking geometry.
bool Character::canMoveTo(Map *map, int nx, int ny)
{
    return !map->isWall(nx, ny);
}

// The character is considered at rest when its interpolated position
// has effectively reached the integer tile coordinates. A small
// epsilon is used to accommodate floating point arithmetic.
bool Character::isAtRest() const
{
    return std::fabs(fx - x) < 0.01f && std::fabs(fy - y) < 0.01f;
}

// Update the logical tile coordinates immediately. `updatePosition`
// will perform the smooth interpolation towards these coordinates.
void Character::moveTo(int nx, int ny)
{
    x = nx;
    y = ny;
}

// Advance interpolated coordinates towards the logical tile
// coordinates using an exponential ease (simple linear interpolation
// towards the target). The `speed` parameter controls the smoothing
// factor: larger values produce faster transitions.
void Character::updatePosition(float speed)
{
    fx += (x - fx) * speed;
    fy += (y - fy) * speed;
    if (std::fabs(fx - x) < 0.01f)
        fx = (float)x;
    if (std::fabs(fy - y) < 0.01f)
        fy = (float)y;
}