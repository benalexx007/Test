#include "mummy.h"
#include "../ai/mummy_ai.h"
#include <cmath>
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

// Set a new AI instance for the mummy. The Mummy takes ownership of
// the provided pointer and deletes any previously installed AI to
// avoid memory leaks.
void Mummy::setAI(IMummyAI* aiInstance) {
    if (ai) {
        delete ai;
    }
    ai = aiInstance;
}

// Execute one discrete movement step. If an AI instance is present
// it is consulted to determine the next tile; otherwise a simple
// greedy heuristic is used to approach the target coordinates.
void Mummy::moveOneStep(Map* map, int targetX, int targetY) {
    // Use AI if available
    if (ai) {
        // Note: the AI's coordinate convention is (row, col) = (y, x)
        std::pair<int, int> mummyPos = {y, x};
        std::pair<int, int> playerPos = {targetY, targetX};

        // If a forced return was previously scheduled (to ensure two
        // distinct visual steps), execute it now and clear the flag.
        if (forcedReturnPending) {
            if (canMoveTo(map, forcedReturnCol, forcedReturnRow)) {
                moveTo(forcedReturnCol, forcedReturnRow);
            }
            forcedReturnPending = false;
            return;
        }

        // Request the AI to compute the next position. The AI may
        // mutate `mummyPos` to indicate its chosen tile.
        std::pair<int, int> orig = mummyPos;
        ai->move(mummyPos, playerPos, map);

        // If the AI elected to remain on its tile, pick a neighbor to
        // step to and schedule a return to the original tile on the
        // following step so that two-step turns remain visually
        // consistent.
        if (mummyPos == orig) {
            const int deltaRow[4] = {1, -1, 0, 0};
            const int deltaCol[4] = {0, 0, 1, -1};
            for (int i = 0; i < 4; ++i) {
                int nr = orig.first + deltaRow[i];
                int nc = orig.second + deltaCol[i];
                if (canMoveTo(map, nc, nr)) {
                    forcedReturnPending = true;
                    forcedReturnCol = orig.second; // x
                    forcedReturnRow = orig.first;  // y
                    mummyPos = {nr, nc};
                    break;
                }
            }
        }

        // Apply the chosen move if the destination is valid.
        if (canMoveTo(map, mummyPos.second, mummyPos.first)) {
            moveTo(mummyPos.second, mummyPos.first);
        }
        return;
    }
    
    // Fallback to a greedy chase algorithm when no AI is installed.
    // This heuristic preferentially moves in the larger component of
    // the delta (x or y) and falls back to orthogonal moves if the
    // preferred direction is blocked.
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

// Convenience: perform two sequential steps. This method is used by
// the higher-level game logic to execute a mummy turn consisting of
// two discrete movement operations.
void Mummy::chase(Map* map, int targetX, int targetY) {
    for (int i = 0; i < 2; ++i) {
        moveOneStep(map, targetX, targetY);
    }
}

AIStateData Mummy::getAIState() {
    if (ai) {
        return ai->getState();
    }
    return AIStateData(); 
}

void Mummy::restoreAIState(const AIStateData& data, Map* map) {
    if (ai) {
        ai->restoreState(data, map);
    }
}