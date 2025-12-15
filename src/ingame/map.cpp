#include "map.h"
#include <fstream>
#include <iostream>

SDL_Texture* Map::loadTexture(const std::string& path) {
    SDL_Texture* tex = IMG_LoadTexture(renderer, path.c_str());
    if (!tex)
        std::cerr << "Failed to load: " << path << " | " << SDL_GetError() << std::endl;
    return tex;
}

Map::Map(SDL_Renderer* ren, char stage) : renderer(ren) {
    tex_floor_light = loadTexture("assets/images/grid/lightGrid" + std::string(1, stage) + ".png");
    tex_floor_dark  = loadTexture("assets/images/grid/darkGrid" + std::string(1, stage) + ".png");
    tex_wall        = loadTexture("assets/images/wall/wall" + std::string(1, stage) + ".png");
}

Map::~Map() {
    SDL_DestroyTexture(tex_floor_light);
    SDL_DestroyTexture(tex_floor_dark);
    SDL_DestroyTexture(tex_wall);
}

void Map::loadFromFile(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) {
        std::cerr << "Error: Could not open map file: " << path << std::endl;
        return;
    }

    grid.clear();
    int val;
    std::vector<int> row;
    while (f >> val) {
        row.push_back(val);
        if (row.size() == 15) { 
            grid.push_back(row);
            row.clear();
        }
    }
    f.close();
}

void Map::render(int offsetX, int offsetY) {
    for (size_t r = 0; r < grid.size(); ++r) {
        for (size_t c = 0; c < grid[r].size(); ++c) {
            SDL_FRect rect = { (float)(c * TILE_SIZE + offsetX), (float)(r * TILE_SIZE + offsetY),
                                (float)TILE_SIZE, (float)TILE_SIZE };

            if ((r + c) % 2 == 0)
                SDL_RenderTexture(renderer, tex_floor_light, NULL, &rect);
            else
                SDL_RenderTexture(renderer, tex_floor_dark, NULL, &rect);

            if (grid[r][c] == 1)
                SDL_RenderTexture(renderer, tex_wall, NULL, &rect);
        }
    }
}

bool Map::isWall(int x, int y) const {
    if (y < 0 || y >= (int)grid.size()) return true;
    if (x < 0 || x >= (int)grid[y].size()) return true;
    return grid[y][x] == 1;
}

int Map::getCols() const {
    return grid.empty() ? 0 : (int)grid[0].size();
}

int Map::getRows() const {
    return (int)grid.size();
}
