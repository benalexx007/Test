#include "mummy_ai.h"
#include "../ingame/map.h"
#include <cstdlib>
#include <ctime>
#include <queue>
#include <algorithm>
#include <cmath>

// EasyAI Implementation
bool EasyAI::isValidPosition(int row, int col, Map* map) {
    if (!map) return false;
    return !map->isWall(col, row); // Map uses (x, y) = (col, row)
}

void EasyAI::move(std::pair<int, int>& mummyPosition,
                  const std::pair<int, int>& playerPosition,
                  Map* map) {
    if (!map) return;
    
    auto [mummyRow, mummyCol] = mummyPosition;
    auto [playerRow, playerCol] = playerPosition;
    
    int distance = abs(mummyRow - playerRow) + abs(mummyCol - playerCol);
    int chanceToChase = 100 - (distance - chaseDistance) * slope;
    if (chanceToChase < 0) chanceToChase = 0;
    if (chanceToChase > 100) chanceToChase = 100;
    
    bool shouldChase = (rand() % 100) < chanceToChase;
    
    // Random Move Logic
    if (!shouldChase) {
        const int deltaRow[] = {1, -1, 0, 0};
        const int deltaCol[] = {0, 0, 1, -1};
        
        int directionIndex = rand() % 4;
        int newRow = mummyRow + deltaRow[directionIndex];
        int newCol = mummyCol + deltaCol[directionIndex];
        
        if (isValidPosition(newRow, newCol, map)) {
            mummyRow = newRow;
            mummyCol = newCol;
        }
        
        mummyPosition = {mummyRow, mummyCol};
        return;
    }
    
    // Simple Direct Chase Logic
    int differenceRow = playerRow - mummyRow;
    int differenceCol = playerCol - mummyCol;
    
    if (rand() & 1) {
        int newRow = mummyRow + (differenceRow > 0 ? 1 : (differenceRow < 0 ? -1 : 0));
        if (isValidPosition(newRow, mummyCol, map)) {
            mummyRow = newRow;
        }
    } else {
        int newCol = mummyCol + (differenceCol > 0 ? 1 : (differenceCol < 0 ? -1 : 0));
        if (isValidPosition(mummyRow, newCol, map)) {
            mummyCol = newCol;
        }
    }
    
    mummyPosition = {mummyRow, mummyCol};
}

// MediumAI Implementation
bool MediumAI::isValidPosition(int row, int col, Map* map) {
    if (!map) return false;
    return !map->isWall(col, row);
}

std::pair<int, int> MediumAI::getNextBfsMove(const std::pair<int, int>& start,
                                             const std::pair<int, int>& goal,
                                             Map* map) {
    if (!map) return start;
    if (start == goal) return start;
    
    int rows = map->getRows();
    int cols = map->getCols();
    
    std::queue<std::pair<int, int>> searchQueue;
    std::vector<std::vector<bool>> visited(rows, std::vector<bool>(cols, false));
    std::vector<std::vector<std::pair<int, int>>> parentMap(rows, 
        std::vector<std::pair<int, int>>(cols, {-1, -1}));
    
    auto [startRow, startCol] = start;
    searchQueue.push({startRow, startCol});
    visited[startRow][startCol] = true;
    
    const int deltaRow[] = {1, -1, 0, 0};
    const int deltaCol[] = {0, 0, 1, -1};
    
    while (!searchQueue.empty()) {
        auto [currentRow, currentCol] = searchQueue.front();
        searchQueue.pop();
        
        if (currentRow == goal.first && currentCol == goal.second) {
            std::pair<int, int> step = goal;
            while (parentMap[step.first][step.second] != start && 
                   parentMap[step.first][step.second] != std::make_pair(-1, -1)) {
                step = parentMap[step.first][step.second];
            }
            if (step == start) return goal;
            return step;
        }
        
        for (int i = 0; i < 4; i++) {
            int newRow = currentRow + deltaRow[i];
            int newCol = currentCol + deltaCol[i];
            
            if (newRow < 0 || newRow >= rows || newCol < 0 || newCol >= cols) continue;
            if (!isValidPosition(newRow, newCol, map) || visited[newRow][newCol]) continue;
            
            visited[newRow][newCol] = true;
            parentMap[newRow][newCol] = {currentRow, currentCol};
            searchQueue.push({newRow, newCol});
        }
    }
    return start;
}

void MediumAI::move(std::pair<int, int>& mummyPosition,
                   const std::pair<int, int>& playerPosition,
                   Map* map) {
    if (!map) return;
    
    auto [mummyRow, mummyCol] = mummyPosition;
    auto [playerRow, playerCol] = playerPosition;
    
    // If recovery mode is active, continue using BFS
    if (bfsModeTurnsRemaining > 0) {
        mummyPosition = getNextBfsMove(mummyPosition, playerPosition, map);
        bfsModeTurnsRemaining--;
        lastPosition = mummyPosition;
        return;
    }
    
    // Basic Greedy Chase Logic
    int differenceRow = playerRow - mummyRow;
    int differenceCol = playerCol - mummyCol;
    
    std::pair<int, int> bestPosition = mummyPosition;
    
    if (abs(differenceRow) >= abs(differenceCol)) {
        int newRow = mummyRow + (differenceRow > 0 ? 1 : (differenceRow < 0 ? -1 : 0));
        if (isValidPosition(newRow, mummyCol, map))
            bestPosition = {newRow, mummyCol};
    } else {
        int newCol = mummyCol + (differenceCol > 0 ? 1 : (differenceCol < 0 ? -1 : 0));
        if (isValidPosition(mummyRow, newCol, map))
            bestPosition = {mummyRow, newCol};
    }
    
    // Oscillation Detection
    if (bestPosition == lastPosition)
        stuckCounter++;
    else
        stuckCounter = 0;
    
    // Activate BFS recovery if stuck
    if (stuckCounter >= 3) {
        bfsModeTurnsRemaining = 3;
        stuckCounter = 0;
        lastPosition = mummyPosition;
        mummyPosition = getNextBfsMove(mummyPosition, playerPosition, map);
        return;
    }
    
    lastPosition = mummyPosition;
    mummyPosition = bestPosition;
}

// HardAI Implementation
HardAI::HardAI(std::pair<int, int> exitPos) : exitPosition(exitPos) {}

bool HardAI::isValidPosition(int row, int col, Map* map) {
    if (!map) return false;
    return !map->isWall(col, row);
}

bool HardAI::hasLineOfSight(std::pair<int, int> mummyPosition, 
                            std::pair<int, int> playerPosition, 
                            Map* map) {
    if (!map) return false;
    if (mummyPosition == playerPosition) return true;
    
    auto [mummyRow, mummyCol] = mummyPosition;
    auto [playerRow, playerCol] = playerPosition;
    
    if (mummyRow == playerRow) {
        int colMin = std::min(mummyCol, playerCol);
        int colMax = std::max(mummyCol, playerCol);
        for (int col = colMin + 1; col < colMax; col++)
            if (map->isWall(col, mummyRow)) return false;
        return true;
    }
    
    if (mummyCol == playerCol) {
        int rowMin = std::min(mummyRow, playerRow);
        int rowMax = std::max(mummyRow, playerRow);
        for (int row = rowMin + 1; row < rowMax; row++)
            if (map->isWall(mummyCol, row)) return false;
        return true;
    }
    
    return false;
}

int HardAI::getBfsDistance(const std::pair<int, int>& start, 
                          const std::pair<int, int>& goal, 
                          Map* map) {
    if (!map) return 9999;
    if (start == goal) return 0;
    
    int rows = map->getRows();
    int cols = map->getCols();
    
    std::queue<std::pair<int, int>> searchQueue;
    std::vector<std::vector<int>> distanceMap(rows, std::vector<int>(cols, -1));
    
    auto [startRow, startCol] = start;
    searchQueue.push({startRow, startCol});
    distanceMap[startRow][startCol] = 0;
    
    const int deltaRow[] = {1, -1, 0, 0};
    const int deltaCol[] = {0, 0, 1, -1};
    
    while (!searchQueue.empty()) {
        auto [currentRow, currentCol] = searchQueue.front();
        searchQueue.pop();
        
        if (currentRow == goal.first && currentCol == goal.second)
            return distanceMap[currentRow][currentCol];
        
        for (int i = 0; i < 4; i++) {
            int newRow = currentRow + deltaRow[i];
            int newCol = currentCol + deltaCol[i];
            
            if (newRow < 0 || newRow >= rows || newCol < 0 || newCol >= cols) continue;
            if (!isValidPosition(newRow, newCol, map) || distanceMap[newRow][newCol] != -1) continue;
            
            distanceMap[newRow][newCol] = distanceMap[currentRow][currentCol] + 1;
            searchQueue.push({newRow, newCol});
        }
    }
    return 9999;
}

std::pair<int, int> HardAI::getNextBfsMove(const std::pair<int, int>& start,
                                           const std::pair<int, int>& goal,
                                           Map* map) {
    if (!map) return start;
    if (start == goal) return start;
    
    int rows = map->getRows();
    int cols = map->getCols();
    
    std::queue<std::pair<int, int>> searchQueue;
    std::vector<std::vector<bool>> visited(rows, std::vector<bool>(cols, false));
    std::vector<std::vector<std::pair<int, int>>> parentMap(rows, 
        std::vector<std::pair<int, int>>(cols, {-1, -1}));
    
    auto [startRow, startCol] = start;
    searchQueue.push({startRow, startCol});
    visited[startRow][startCol] = true;
    
    const int deltaRow[] = {1, -1, 0, 0};
    const int deltaCol[] = {0, 0, 1, -1};
    
    while (!searchQueue.empty()) {
        auto [currentRow, currentCol] = searchQueue.front();
        searchQueue.pop();
        
        if (currentRow == goal.first && currentCol == goal.second) {
            std::pair<int, int> step = goal;
            while (parentMap[step.first][step.second] != start && 
                   parentMap[step.first][step.second] != std::make_pair(-1, -1)) {
                step = parentMap[step.first][step.second];
            }
            if (step == start) return goal;
            return step;
        }
        
        for (int i = 0; i < 4; i++) {
            int newRow = currentRow + deltaRow[i];
            int newCol = currentCol + deltaCol[i];
            
            if (newRow < 0 || newRow >= rows || newCol < 0 || newCol >= cols) continue;
            if (!isValidPosition(newRow, newCol, map) || visited[newRow][newCol]) continue;
            
            visited[newRow][newCol] = true;
            parentMap[newRow][newCol] = {currentRow, currentCol};
            searchQueue.push({newRow, newCol});
        }
    }
    return start;
}

std::pair<int, int> HardAI::predictPlayerPosition(const std::pair<int, int>& playerPosition) {
    if (lastPlayerPosition.first == -1) return playerPosition;
    
    auto [playerRow, playerCol] = playerPosition;
    auto [lastRow, lastCol] = lastPlayerPosition;
    
    return {playerRow + (playerRow - lastRow), playerCol + (playerCol - lastCol)};
}

void HardAI::generateSpiralSearch(const std::pair<int, int>& center, Map* map) {
    if (!map) return;
    searchPath.clear();
    searchPathIndex = 0;
    searchCenter = center;
    auto [centerRow, centerCol] = center;
    
    for (int radius = 1; radius <= searchRadius; radius++) {
        for (int c = centerCol - radius; c <= centerCol + radius; c++)
            if (isValidPosition(centerRow - radius, c, map)) 
                searchPath.push_back({centerRow - radius, c});
        
        for (int r = centerRow - radius + 1; r <= centerRow + radius; r++)
            if (isValidPosition(r, centerCol + radius, map)) 
                searchPath.push_back({r, centerCol + radius});
        
        for (int c = centerCol + radius - 1; c >= centerCol - radius; c--)
            if (isValidPosition(centerRow + radius, c, map)) 
                searchPath.push_back({centerRow + radius, c});
        
        for (int r = centerRow + radius - 1; r > centerRow - radius; r--)
            if (isValidPosition(r, centerCol - radius, map)) 
                searchPath.push_back({r, centerCol - radius});
    }
}

std::pair<int, int> HardAI::findInterceptPoint(const std::pair<int, int>& mummyPosition,
                                               const std::pair<int, int>& suspectedPlayerPosition,
                                               Map* map) {
    if (!map) return mummyPosition;
    
    int rows = map->getRows();
    int cols = map->getCols();
    
    std::pair<int, int> bestPoint = mummyPosition;
    int bestScore = -999999;
    
    auto [suspectRow, suspectCol] = suspectedPlayerPosition;
    auto [exitRow, exitCol] = exitPosition;
    
    int minRow = std::min(suspectRow, exitRow);
    int maxRow = std::max(suspectRow, exitRow);
    int minCol = std::min(suspectCol, exitCol);
    int maxCol = std::max(suspectCol, exitCol);
    
    for (int row = std::max(1, minRow - 2); row <= std::min(rows - 2, maxRow + 2); row++) {
        for (int col = std::max(1, minCol - 2); col <= std::min(cols - 2, maxCol + 2); col++) {
            if (!isValidPosition(row, col, map)) continue;
            
            int distMummy = getBfsDistance(mummyPosition, {row, col}, map);
            int distPlayer = getBfsDistance(suspectedPlayerPosition, {row, col}, map);
            int distExit = getBfsDistance({row, col}, exitPosition, map);
            
            int score = (distPlayer - distMummy) * 10 - distExit;
            
            if (score > bestScore) {
                bestScore = score;
                bestPoint = {row, col};
            }
        }
    }
    return bestPoint;
}

void HardAI::move(std::pair<int, int>& mummyPosition,
                 const std::pair<int, int>& playerPosition,
                 Map* map) {
    if (!map) return;
    
    bool canSeePlayerBefore = hasLineOfSight(mummyPosition, playerPosition, map);
    
    int rows = map->getRows();
    int cols = map->getCols();
    
    // Immediate reaction if player is seen
    if (canSeePlayerBefore) {
        currentState = HUNT;
        lastKnownPosition = playerPosition;
        lastPlayerPosition = playerPosition;
        turnsNotSeenCounter = 0;
        
        mummyPosition = getNextBfsMove(mummyPosition, playerPosition, map);
        
        if (hasLineOfSight(mummyPosition, playerPosition, map)) {
            lastKnownPosition = playerPosition;
            currentState = HUNT;
            turnsNotSeenCounter = 0;
        }
        return;
    }
    
    turnsNotSeenCounter++;
    
    // State Transitions
    if (currentState == HUNT && turnsNotSeenCounter >= 2) {
        currentState = SEARCH;
        generateSpiralSearch(lastKnownPosition, map);
    } else if (currentState == SEARCH && turnsNotSeenCounter >= 8) {
        currentState = INTERCEPT;
    } else if (currentState == INTERCEPT && turnsNotSeenCounter >= 15) {
        currentState = PATROL;
    }
    
    // Execute logic based on current state
    switch (currentState) {
    case HUNT:
        if (mummyPosition == lastKnownPosition) {
            currentState = SEARCH;
            generateSpiralSearch(lastKnownPosition, map);
        } else {
            mummyPosition = getNextBfsMove(mummyPosition, lastKnownPosition, map);
        }
        break;
    
    case SEARCH:
        if (searchPath.empty() || searchPathIndex >= (int)searchPath.size()) {
            currentState = INTERCEPT;
            break;
        }
        
        if (mummyPosition == searchPath[searchPathIndex]) {
            searchPathIndex++;
            if (searchPathIndex < (int)searchPath.size())
                mummyPosition = getNextBfsMove(mummyPosition, searchPath[searchPathIndex], map);
        } else {
            mummyPosition = getNextBfsMove(mummyPosition, searchPath[searchPathIndex], map);
        }
        break;
    
    case INTERCEPT: {
        std::pair<int, int> suspectedPos = lastKnownPosition;
        if (lastPlayerPosition.first != -1) {
            suspectedPos = predictPlayerPosition(lastKnownPosition);
            suspectedPos.first = std::max(1, std::min(rows - 2, suspectedPos.first));
            suspectedPos.second = std::max(1, std::min(cols - 2, suspectedPos.second));
        }
        
        std::pair<int, int> interceptPt = findInterceptPoint(mummyPosition, suspectedPos, map);
        
        if (mummyPosition == interceptPt) {
            if (getBfsDistance(mummyPosition, exitPosition, map) > 3) {
                mummyPosition = getNextBfsMove(mummyPosition, exitPosition, map);
            }
        } else {
            mummyPosition = getNextBfsMove(mummyPosition, interceptPt, map);
        }
        break;
    }
    
    case PATROL: {
        std::pair<int, int> centerMap = {rows / 2, cols / 2};
        std::pair<int, int> target = (turnsNotSeenCounter % 10 < 5) ? exitPosition : centerMap;
        
        if (mummyPosition == target)
            target = (target == exitPosition) ? centerMap : exitPosition;
        
        mummyPosition = getNextBfsMove(mummyPosition, target, map);
        break;
    }
    }
    
    lastPlayerPosition = playerPosition;
    
    // Check visibility again during movement
    if (hasLineOfSight(mummyPosition, playerPosition, map)) {
        currentState = HUNT;
        lastKnownPosition = playerPosition;
        lastPlayerPosition = playerPosition;
        turnsNotSeenCounter = 0;
    }
}

