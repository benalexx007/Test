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
    tex_exit        = loadTexture("assets/images/grid/exit1.jpg");
}

Map::~Map() {
    SDL_DestroyTexture(tex_floor_light);
    SDL_DestroyTexture(tex_floor_dark);
    SDL_DestroyTexture(tex_wall);
    SDL_DestroyTexture(tex_exit);
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
            if (grid[r][c] == 4)
                SDL_RenderTexture(renderer, tex_exit, NULL, &rect);
        }
    }
}

bool Map::isWall(int x, int y) const {
    if (y < 0 || y >= (int)grid.size()) return true;
    if (x < 0 || x >= (int)grid[y].size()) return true;
    return grid[y][x] == 1;
}

bool Map::isExit(int x, int y) const {
    if (y < 0 || y >= (int)grid.size()) return false;
    if (x < 0 || x >= (int)grid[y].size()) return false;
    return grid[y][x] == 4;
}

void Map::getExitPosition(int& exitX, int& exitY) const {
    exitX = -1;
    exitY = -1;
    for (size_t r = 0; r < grid.size(); ++r) {
        for (size_t c = 0; c < grid[r].size(); ++c) {
            if (grid[r][c] == 4) {
                exitX = (int)c;
                exitY = (int)r;
                return;
            }
        }
    }
}

void Map::getExplorerPosition(int& expX, int& expY) const {
    expX = -1;
    expY = -1;
    for (size_t r = 0; r < grid.size(); ++r) {
        for (size_t c = 0; c < grid[r].size(); ++c) {
            if (grid[r][c] == 3) {
                expX = (int)c;
                expY = (int)r;
                return;
            }
        }
    }
}

void Map::getMummyPosition(int& mummyX, int& mummyY) const {
    mummyX = -1;
    mummyY = -1;
    for (size_t r = 0; r < grid.size(); ++r) {
        for (size_t c = 0; c < grid[r].size(); ++c) {
            if (grid[r][c] == 2) {
                mummyX = (int)c;
                mummyY = (int)r;
                return;
            }
        }
    }
}

int Map::getCols() const {
    return grid.empty() ? 0 : (int)grid[0].size();
}

int Map::getRows() const {
    return (int)grid.size();
}
