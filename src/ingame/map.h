#pragma once
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <vector>
#include <string>

class Map {
private:
    SDL_Renderer* renderer;
    SDL_Texture* tex_floor_light;
    SDL_Texture* tex_floor_dark;
    SDL_Texture* tex_wall;
    std::vector<std::vector<int>> grid;
    int TILE_SIZE = 64;

    SDL_Texture* loadTexture(const std::string& path);

public:
    Map(SDL_Renderer* ren, char stage);
    ~Map();

    void loadFromFile(const std::string& path);
    void render(int offsetX, int offsetY);
    bool isWall(int x, int y) const;
    int getCols() const;
    int getRows() const;
    int getTileSize() const { return TILE_SIZE; }
};
