#include "map.h"
#include <fstream>
#include <iostream>
#include <sstream>

// Map implementation notes:
// - The `Map` stores a 2D integer grid (`grid`) where each cell is a
//   small integer code representing tile semantics used by the game:
//     0 = empty floor
//     1 = wall (non-walkable)
//     2 = mummy start position
//     3 = explorer (player) start position
//     4 = exit tile
// - Coordinate convention used throughout this file and the rest of
//   the project: functions that accept (x, y) treat `x` as column
//   (horizontal) and `y` as row (vertical). Internally `grid` is
//   indexed as `grid[row][col]` so callers must flip indices when
//   accessing `grid` directly.
// - `TILE_SIZE` is the pixel dimension of each tile used for
//   rendering; `render()` maps grid coordinates to pixel-space by
//   multiplying tile indices by `TILE_SIZE`.
// - The Map class also caches SDL textures for floor, wall and exit
//   tiles so that rendering is a simple texture blit per-cell.


// Load an SDL texture from `path` using the `renderer` associated with
// this Map. Returns nullptr on failure and logs a descriptive
// message to `stderr` for debugging.
SDL_Texture* Map::loadTexture(const std::string& path) {
    SDL_Texture* tex = IMG_LoadTexture(renderer, path.c_str());
    if (!tex)
        std::cerr << "Failed to load: " << path << " | " << SDL_GetError() << std::endl;
    return tex;
}

// Construct a Map and preload stage-specific textures.
// `stage` is a single character used to choose stage-specific images
// (e.g. '1','2','3'). Textures are cached as members to avoid reloading
// them each frame.
Map::Map(SDL_Renderer* ren, char stage) : renderer(ren) {
    tex_floor_light = loadTexture("assets/images/grid/lightGrid" + std::string(1, stage) + ".png");
    tex_floor_dark  = loadTexture("assets/images/grid/darkGrid" + std::string(1, stage) + ".png");
    tex_wall        = loadTexture("assets/images/wall/wall" + std::string(1, stage) + ".png");
    tex_exit        = loadTexture("assets/images/grid/exit" + std::string(1, stage) + ".jpg");
}

// Destroy cached textures when the Map is torn down.
Map::~Map() {
    SDL_DestroyTexture(tex_floor_light);
    SDL_DestroyTexture(tex_floor_dark);
    SDL_DestroyTexture(tex_wall);
    SDL_DestroyTexture(tex_exit);
}

// Parse a whitespace-separated integer map file. Each non-empty line
// corresponds to one row; integers are read left-to-right as columns.
// The method clears any existing grid data before loading. The file
// format is simple and forgiving: extra whitespace is ignored, and
// empty lines are skipped. This keeps the level format human-editable.
void Map::loadFromFile(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) {
        std::cerr << "Error: Could not open map file: " << path << std::endl;
        return;
    }

    grid.clear();
    std::string line;
    while (std::getline(f, line)) {
        if (line.empty()) continue;
        std::vector<int> row;
        std::istringstream iss(line);
        int val;
        while (iss >> val) {
            row.push_back(val);
        }
        if (!row.empty()) {
            grid.push_back(row);
        }
    }
    f.close();
}

// Render the whole tile grid. `offsetX`/`offsetY` are pixel offsets
// used to position the map inside the game window (useful for
// camera movement). The function draws a subtle checkerboard of two
// floor textures for visual variety and then overlays wall/exit
// textures on top of floor tiles where appropriate.
//
// Performance note: the current implementation performs a texture
// blit per tile each frame. This is simple and predictable; if
// profiling shows this is a bottleneck for large maps, consider
// batching tiles into a single render target or culling invisible
// tiles based on the viewport.
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

// Return true if the tile at (x, y) is a wall or outside the map
// bounds. Out-of-bounds is considered a wall to simplify collision
// checks in callers.
bool Map::isWall(int x, int y) const {
    if (y < 0 || y >= (int)grid.size()) return true;
    if (x < 0 || x >= (int)grid[y].size()) return true;
    return grid[y][x] == 1;
}

// Return true if the tile at (x, y) is the exit tile.
bool Map::isExit(int x, int y) const {
    if (y < 0 || y >= (int)grid.size()) return false;
    if (x < 0 || x >= (int)grid[y].size()) return false;
    return grid[y][x] == 4;
}

// Find the first exit tile in the grid and return its coordinates via
// `exitX`/`exitY`. If no exit is found both outputs are set to -1.
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

// Find the explorer (player) start position in the grid. Returns
// (-1,-1) when not present.
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

// Find the mummy start position in the grid. Returns (-1,-1) when
// not present.
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

// Utility queries for grid dimensions.
int Map::getCols() const {
    return grid.empty() ? 0 : (int)grid[0].size();
}

int Map::getRows() const {
    return (int)grid.size();
}
