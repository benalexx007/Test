#include <SDL3/SDL.h>
#include <SDL3/SDL_image.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>

class Map {
private:
    SDL_Renderer* renderer;
    SDL_Texture* tex_floor_light;
    SDL_Texture* tex_floor_dark;
    SDL_Texture* tex_wall;

    std::vector<std::vector<int>> grid;
    const int TILE_SIZE = 64;

    // Hàm tải ảnh
    SDL_Texture* loadTexture(const std::string& path) {
        SDL_Texture* tex = IMG_LoadTexture(renderer, path.c_str());
        if (!tex)
            std::cerr << "Failed to load: " << path << " | " << SDL_GetError() << std::endl;
        return tex;
    }

public:
    Map(SDL_Renderer* ren) : renderer(ren) {
        tex_floor_light = loadTexture("assets/images/maysang.png");
        tex_floor_dark  = loadTexture("assets/images/maytoi.png");
        tex_wall        = loadTexture("assets/images/set.png");

        if (!tex_floor_light || !tex_floor_dark || !tex_wall)
            std::cerr << "⚠️ One or more textures failed to load!\n";
    }

    ~Map() {
        SDL_DestroyTexture(tex_floor_light);
        SDL_DestroyTexture(tex_floor_dark);
        SDL_DestroyTexture(tex_wall);
    }

    void loadFromFile(const std::string& path) {
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
            if (row.size() == 10) {
                grid.push_back(row);
                row.clear();
            }
        }
        f.close();

        std::cout << "Loaded map size: " << grid.size() 
                  << "x" << (grid.empty() ? 0 : grid[0].size()) << std::endl;
    }

    void render() {
        for (size_t r = 0; r < grid.size(); ++r) {
            for (size_t c = 0; c < grid[r].size(); ++c) {
                SDL_FRect rect = {
                    (float)(c * TILE_SIZE),
                    (float)(r * TILE_SIZE),
                    (float)TILE_SIZE,
                    (float)TILE_SIZE
                };

                // Vẽ nền
                if ((r + c) % 2 == 0)
                    SDL_RenderTexture(renderer, tex_floor_light, NULL, &rect);
                else
                    SDL_RenderTexture(renderer, tex_floor_dark, NULL, &rect);

                // Vẽ tường đè lên
                if (grid[r][c] == 1)
                    SDL_RenderTexture(renderer, tex_wall, NULL, &rect);
            }
        }
    }
};
