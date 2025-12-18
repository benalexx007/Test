#include "mummy.h"
#include "../ai/mummy_ai.h"
#include <cmath>

Mummy::Mummy(SDL_Renderer* renderer, int startX, int startY, int tileSize, char stage)
    : Character(renderer, "assets/images/mummy/mummy", std::string(1, stage), startX, startY, tileSize), ai(nullptr) {}

Mummy::~Mummy() {
    if (ai) {
        delete ai;
        ai = nullptr;
    }
}

void Mummy::setAI(IMummyAI* aiInstance) {
    if (ai) {
        delete ai;
    }
    ai = aiInstance;
}

void Mummy::moveOneStep(Map* map, int targetX, int targetY) {
    // Use AI if available
    if (ai) {
        std::pair<int, int> mummyPos = {y, x}; // Note: AI uses (row, col) = (y, x)
        std::pair<int, int> playerPos = {targetY, targetX};
        ai->move(mummyPos, playerPos, map);
        // Update position from AI result
        if (canMoveTo(map, mummyPos.second, mummyPos.first)) {
            moveTo(mummyPos.second, mummyPos.first);
        }
        return;
    }
    
    // Fallback to original greedy chase if no AI
    int dx = targetX - x;
    int dy = targetY - y;

    int dirs[4][2];
    if (std::abs(dx) > std::abs(dy)) {
        dirs[0][0] = (dx > 0) ? 1 : -1; dirs[0][1] = 0;
        dirs[1][0] = 0; dirs[1][1] = (dy > 0) ? 1 : -1;
        dirs[2][0] = 0; dirs[2][1] = -(dirs[1][1]);
        dirs[3][0] = -(dirs[0][0]); dirs[3][1] = 0;
    } else {
        dirs[0][0] = 0; dirs[0][1] = (dy > 0) ? 1 : -1;
        dirs[1][0] = (dx > 0) ? 1 : -1; dirs[1][1] = 0;
        dirs[2][0] = -(dirs[1][0]); dirs[2][1] = 0;
        dirs[3][0] = 0; dirs[3][1] = -(dirs[0][1]);
    }

    for (int i = 0; i < 4; ++i) {
        int nx = x + dirs[i][0];
        int ny = y + dirs[i][1];
        if (canMoveTo(map, nx, ny)) {
            moveTo(nx, ny);
            return;
        }
    }
}

void Mummy::chase(Map* map, int targetX, int targetY) {
    for (int i = 0; i < 2; ++i) {
        moveOneStep(map, targetX, targetY);
    }
}