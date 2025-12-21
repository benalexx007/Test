#pragma once
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <vector>
#include <string>

class Map {
private:
    SDL_Renderer* renderer; // Renderer used for creating textures
    SDL_Texture* tex_floor_light; // light floor tile texture
    SDL_Texture* tex_floor_dark;  // dark floor tile texture
    SDL_Texture* tex_wall;        // wall tile texture
    SDL_Texture* tex_exit;        // exit tile texture

    // Tile grid: grid[row][col] indexing. Each integer encodes a
    // semantic tile type (0=floor, 1=wall, 2=mummy, 3=explorer, 4=exit).
    std::vector<std::vector<int>> grid;

    // Pixel size of a single tile used for rendering and hit tests.
    int TILE_SIZE = 64;

    // Helper to load and return an SDL_Texture from an image path.
    SDL_Texture* loadTexture(const std::string& path);

public:
    // Construct a Map and preload stage-specific textures. `stage`
    // selects which tile art to load (e.g. '1','2','3').
    Map(SDL_Renderer* ren, char stage);
    ~Map();

    // Load a whitespace-separated integer grid from a file. Each
    // non-empty line becomes one row; whitespace separates columns.
    void loadFromFile(const std::string& path);

    // Render the tile grid at pixel offset (offsetX, offsetY).
    void render(int offsetX, int offsetY);

    // Return true if (x,y) is a wall or out-of-bounds (treated as wall).
    bool isWall(int x, int y) const;

    // Grid dimension helpers
    int getCols() const;
    int getRows() const;
    int getTileSize() const { return TILE_SIZE; }

    // Return true if (x,y) is the exit tile.
    bool isExit(int x, int y) const;

    // Helper queries: find the first matching tile for exit, explorer,
    // and mummy. If not found coordinates are set to (-1,-1).
    void getExitPosition(int& exitX, int& exitY) const;
    void getExplorerPosition(int& expX, int& expY) const;
    void getMummyPosition(int& mummyX, int& mummyY) const;
};
