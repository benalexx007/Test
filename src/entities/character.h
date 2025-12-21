// Base class for movable entities (Explorer and Mummy). This class
// provides common functionality such as texture loading, tile-based
// positioning, and smooth interpolation between tile positions for
// animation.
#pragma once
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <string>
#include "../ingame/map.h"

class Character {
protected:
    SDL_Renderer* renderer; // renderer used to draw the character texture
    SDL_Texture* texture;   // loaded sprite texture
    int x, y;               // logical tile coordinates
    int tileSize;           // size of a map tile in pixels

    // `fx` and `fy` store interpolated (floating-point) coordinates
    // used to smoothly animate movement between integer tile
    // positions. The rendering code uses `fx`/`fy` to compute pixel
    // coordinates for the texture.
    float fx, fy;

public:
    // Constructor expects a renderer, a base path for the image file
    // (without stage suffix), a stage string to select the sprite,
    // and initial tile coordinates. The implementation loads a PNG
    // texture using this information.
    Character(SDL_Renderer* renderer, const std::string& baseName, const std::string& stage, int startX, int startY, int tileSize);
    virtual ~Character();

    // Render the character at its interpolated position. `offsetX` and
    // `offsetY` transform tile coordinates into viewport pixel space.
    virtual void render(int offsetX = 0, int offsetY = 0);

    // Movement helpers: test whether a tile is accessible, set the
    // logical position, query rest state, and advance interpolation.
    bool canMoveTo(Map* map, int nx, int ny);
    void moveTo(int nx, int ny);
    bool isAtRest() const;
    void updatePosition(float speed = 0.2f); // call each frame to tween

    int getX() const { return x; }
    int getY() const { return y; }
    
};