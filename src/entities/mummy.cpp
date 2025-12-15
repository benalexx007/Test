#include "mummy.h"
#include <cmath>

Mummy::Mummy(SDL_Renderer* renderer, int startX, int startY, int tileSize, char stage)
    : Character(renderer, "assets/images/mummy/mummy", std::string(1, stage), startX, startY, tileSize) {}

void Mummy::moveOneStep(Map* map, int targetX, int targetY) {
    int dx = targetX - x;
    int dy = targetY - y;

    int dirs[4][2];
    if (std::abs(dx) > std::abs(dy)) {
        dirs[0][0] = (dx > 0) ? 1 : -1; dirs[0][1] = 0;                 // ưu tiên trục X
        dirs[1][0] = 0; dirs[1][1] = (dy > 0) ? 1 : -1;                  // sau đó trục Y
        dirs[2][0] = 0; dirs[2][1] = -(dirs[1][1]);                      // Y ngược
        dirs[3][0] = -(dirs[0][0]); dirs[3][1] = 0;                      // X ngược
    } else {
        dirs[0][0] = 0; dirs[0][1] = (dy > 0) ? 1 : -1;                  // ưu tiên trục Y
        dirs[1][0] = (dx > 0) ? 1 : -1; dirs[1][1] = 0;                  // sau đó trục X
        dirs[2][0] = -(dirs[1][0]); dirs[2][1] = 0;                      // X ngược
        dirs[3][0] = 0; dirs[3][1] = -(dirs[0][1]);                      // Y ngược
    }

    for (int i = 0; i < 4; ++i) {
        int nx = x + dirs[i][0];
        int ny = y + dirs[i][1];
        if (canMoveTo(map, nx, ny)) {
            moveTo(nx, ny);
            return;
        }
    }
    // nếu bí bách xung quanh đều là tường thì đứng im
}

void Mummy::chase(Map* map, int targetX, int targetY) {
    // đi 2 bước mỗi lượt, không delay để không chặn render
    for (int i = 0; i < 2; ++i) {
        moveOneStep(map, targetX, targetY);
    }
}