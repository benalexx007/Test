#include "mummy_ai.h"
#include "../ingame/map.h"
#include <cstdlib>
#include <ctime>
#include <queue>
#include <algorithm>
#include <cmath>

// EasyAI Implementation
// ----------------------
// The easiest strategy: probabilistic pursuit mixed with random
// wandering. This AI computes a simple Manhattan-distance-based
// probability of initiating a chase; when it elects not to chase it
// performs a single-step random neighbor move. The design goal is to
// produce non-deterministic, obviously suboptimal behaviour suited
// for an easy difficulty setting.
//
// Coordinate convention: AI methods use (row, col) pairs where
// `row` corresponds to the map Y coordinate and `col` to the X
// coordinate. The `Map` API exposes `isWall(x,y)` in (col,row)
// order, so callers must flip the pair when invoking Map methods.
//
// Complexity: O(1) per `move()` call (constant-time checks only).
bool EasyAI::isValidPosition(int row, int col, Map* map) {
    if (!map) return false;
    // Note: Map expects coordinates in (x, y) order for its methods.
    return !map->isWall(col, row); // Map uses (x, y) = (col, row)
}

// Decide the mummy's next tile and write it into `mummyPosition`.
// Parameters:
// - mummyPosition : current (row,col) of the mummy; mutated in-place
// - playerPosition: current (row,col) of the player
// - map           : pointer to the world `Map` for collision queries
//
// Behaviour summary:
// 1. Compute a chase probability inversely proportional to manhattan
//    distance (closer => more likely to chase).
// 2. With the complement probability perform a single-step random
//    walk to give the AI a wandering personality.
// 3. If chasing, prefer moving one step in either the row or column
//    direction chosen at random (simple greedy step). No global path
//    planning is performed.
//
// Rationale: this produces simple, inexpensive behaviour that is
// intentionally easy for the player to outwit.

EasyAI::EasyAI() {
    currentSeed = (unsigned int)time(nullptr);
}

int EasyAI::myRand() {
    currentSeed = currentSeed * 1103515245 + 12345;
    return (unsigned int)(currentSeed / 65536) % 32768;
}

void EasyAI::move(std::pair<int, int>& mummyPosition,
                  const std::pair<int, int>& playerPosition,
                  Map* map) {
    if (!map) return;
    
    auto [mummyRow, mummyCol] = mummyPosition;
    auto [playerRow, playerCol] = playerPosition;
    
    // Compute a simple Manhattan distance-based chase probability.
    int distance = abs(mummyRow - playerRow) + abs(mummyCol - playerCol);
    int chanceToChase = 100 - (distance - chaseDistance) * slope;
    if (chanceToChase < 0) chanceToChase = 0;
    if (chanceToChase > 100) chanceToChase = 100;
    
    bool shouldChase = (myRand() % 100) < chanceToChase;
    
    // Random Move Logic when not chasing
    if (!shouldChase) {
        const int deltaRow[] = {1, -1, 0, 0};
        const int deltaCol[] = {0, 0, 1, -1};
        
        int directionIndex = myRand() % 4;
        int newRow = mummyRow + deltaRow[directionIndex];
        int newCol = mummyCol + deltaCol[directionIndex];
        
        if (isValidPosition(newRow, newCol, map)) {
            mummyRow = newRow;
            mummyCol = newCol;
        }
        
        mummyPosition = {mummyRow, mummyCol};
        return;
    }
    
    // Simple Direct Chase Logic when the AI decides to pursue the player.
    int differenceRow = playerRow - mummyRow;
    int differenceCol = playerCol - mummyCol;
    
    if (myRand() & 1) {
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
// -----------------------
// Medium difficulty blends a greedy strategy with short-range BFS
// recovery when oscillation or simple traps are detected. This makes
// the AI frequently effective but still fallible and computationally
// lightweight.
//
// Key behaviours:
// - Greedy step toward the player along the larger coordinate delta.
// - Detect repeated back-and-forth (oscillation) using `lastPosition`
//   and `stuckCounter` and switch to BFS-based recovery mode for a
//   few turns to escape local traps.
//
// Complexity:
// - Greedy updates are O(1).
// - BFS recovery is O(R*C) in the worst case where R/C are map
//   dimensions; BFS is bounded by the map size but invoked only when
//   recovery is necessary, keeping average cost low.
bool MediumAI::isValidPosition(int row, int col, Map* map) {
    if (!map) return false;
    return !map->isWall(col, row);
}

// Breadth-first search helper that returns the first step along the
// shortest path from `start` to `goal`. If no path exists the
// function returns `start`.
// Compute the first step along a shortest path from `start` to
// `goal` using breadth-first search. The function returns `start`
// when no path exists or when `start==goal`.
//
// Notes:
// - `start` and `goal` are (row, col) pairs. `Map` methods require
//   (col,row) ordering, so calls are translated accordingly.
// - The implementation stores a parent pointer for each visited cell
//   and backtracks from `goal` to locate the immediate successor of
//   `start` on the shortest path.
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

// Primary MediumAI move function: use greedy movement towards the
// player but switch into BFS recovery mode if oscillation or getting
// stuck is detected. BFS recovery computes short term paths to the
// player to escape local traps.
// Main MediumAI step function. If `bfsModeTurnsRemaining>0` the
// AI continues to follow BFS moves computed earlier; otherwise it
// attempts a greedy step and may trigger BFS recovery if oscillation
// is detected.
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
    
    // Oscillation Detection: if the chosen position equals the
    // previous position repeatedly, increment `stuckCounter`.
    if (bestPosition == lastPosition)
        stuckCounter++;
    else
        stuckCounter = 0;
    
    // Activate BFS recovery if stuck for several turns.
    if (stuckCounter >= 2) {
        bfsModeTurnsRemaining = 2;
        stuckCounter = 0;
        lastPosition = mummyPosition;
        mummyPosition = getNextBfsMove(mummyPosition, playerPosition, map);
        return;
    }
    
    lastPosition = mummyPosition;
    mummyPosition = bestPosition;
}

// HardAI Implementation
// ----------------------
// Hard difficulty implements a small finite-state machine (FSM)
// combining direct pursuit, localized spiral search, predictive
// interception, and patrol behaviours. It uses BFS pathfinding for
// precise navigation and additional heuristics to choose tactical
// intercept points that attempt to cut off the player's path to the
// exit.
//
// States:
// - HUNT      : direct BFS pursuit when the player is visible
// - SEARCH    : spiral search around the last known position
// - INTERCEPT : predict player motion and choose an intercept waypoint
// - PATROL    : fallback movement between the map center and exit
//
// Complexity:
// - Visibility checks are O(d) along straight lines.
// - BFS calls are O(R*C) worst-case and are selectively invoked.
//
HardAI::HardAI(std::pair<int, int> exitPos) : exitPosition(exitPos) {}

bool HardAI::isValidPosition(int row, int col, Map* map) {
    if (!map) return false;
    return !map->isWall(col, row);
}

// Determine whether the mummy has a line of sight (straight-line
// unobstructed path) to the player along rows or columns. This is a
// simple visibility test used to transition between states.
// Determine unobstructed straight-line visibility along rows or
// columns. Returns true only if the mummy and player are aligned on
// the same row or the same column and no wall tiles intervene.
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

// Compute the BFS distance between two grid cells. Returns a large
// sentinel value if no path exists. This function is used by the
// HardAI's heuristic computations.
// Compute the BFS distance (number of steps) between two cells. If
// no path exists returns a large sentinel value. This is used by the
// intercept heuristic to estimate relative reachability.
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

// Find the first step along a shortest path computed by BFS. This
// function performs a standard BFS and then backtracks to determine
// the first step from `start` toward `goal`.
// Return the first step on a shortest path from `start` to `goal`,
// computed via BFS. See MediumAI::getNextBfsMove for implementation
// details; this is a duplicate helper specialized for the HardAI
// module to keep the semantics explicit in this translation unit.
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

// Predict the player's future position using their last known
// movement vector. This simple linear extrapolation is used by the
// INTERCEPT state.
// Linearly extrapolate the player's position using the previous
// observed movement vector (player - lastPlayerPosition). This is a
// very simple predictive model that assumes constant velocity and is
// intentionally conservative.
std::pair<int, int> HardAI::predictPlayerPosition(const std::pair<int, int>& playerPosition) {
    if (lastPlayerPosition.first == -1) return playerPosition;
    
    auto [playerRow, playerCol] = playerPosition;
    auto [lastRow, lastCol] = lastPlayerPosition;
    
    return {playerRow + (playerRow - lastRow), playerCol + (playerCol - lastCol)};
}

// Generate a spiral sequence of search cells around `center` used by
// the SEARCH state. The spiral is limited by `searchRadius` and only
// includes walkable cells.
// Produce a deterministic spiral of candidate cells around `center`
// up to `searchRadius`. The spiral order is used by the SEARCH state
// to methodically probe neighbouring tiles.
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

// Heuristic function that chooses an intercept point between the
// suspected player position and the exit. The scoring function prefers
// cells where the player-to-point distance is larger than the mummy's
// distance, weighted by an exit proximity penalty.
// Heuristic search for a tactical intercept waypoint. The function
// examines a small bounding box between the suspected player
// position and the exit and scores candidates according to the
// difference in BFS distances (player->cell minus mummy->cell) and
// proximity to the exit. Higher scores prefer cells where the mummy
// can reach relatively sooner than the player while also staying
// close enough to the exit to protect it.
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

// Hard AI state-machine. The AI transitions between HUNT (direct
// pursuit when the player is visible), SEARCH (local spiral search
// around last known player position), INTERCEPT (predictive chase),
// and PATROL (area coverage when the player is not recently seen).
// Hard AI state-machine: decide next action based on recent sighting
// history and current state. This function encapsulates the full
// behavioural logic of the `HardAI` including transitions between
// HUNT, SEARCH, INTERCEPT, and PATROL, and the conditions that
// trigger those transitions.
//
// The algorithm maintains internal memory of the last known player
// position and the last observed player velocity to enable
// predictive intercepts. It deliberately balances precise BFS-based
// navigation with higher-level strategic choices.
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
    
    // State Transitions based on time since last sighting
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
    
    // Check visibility again during movement and transition to HUNT
    if (hasLineOfSight(mummyPosition, playerPosition, map)) {
        currentState = HUNT;
        lastKnownPosition = playerPosition;
        lastPlayerPosition = playerPosition;
        turnsNotSeenCounter = 0;
    }
}

// Save snapshot
AIStateData EasyAI::getState() {
    AIStateData data;
    data.rngSeed = this->currentSeed; 
    return data;
}

void EasyAI::restoreState(const AIStateData& data, Map* map) {
    this->currentSeed = data.rngSeed; 
}

AIStateData MediumAI::getState() {
    AIStateData data;
    data.stuckCounter = this->stuckCounter;
    data.bfsModeTurnsRemaining = this->bfsModeTurnsRemaining;
    data.lastPosX = this->lastPosition.first;
    data.lastPosY = this->lastPosition.second;
    return data;
}

void MediumAI::restoreState(const AIStateData& data, Map* map) {
    this->stuckCounter = data.stuckCounter;
    this->bfsModeTurnsRemaining = data.bfsModeTurnsRemaining;
    this->lastPosition = {data.lastPosX, data.lastPosY};
}

AIStateData HardAI::getState() {
    AIStateData data;
    data.state = (int)this->currentState;
    data.lastKnownPosX = this->lastKnownPosition.first;
    data.lastKnownPosY = this->lastKnownPosition.second;
    data.lastPlayerPosX = this->lastPlayerPosition.first;
    data.lastPlayerPosY = this->lastPlayerPosition.second;
    data.turnsNotSeenCounter = this->turnsNotSeenCounter;
    data.searchCenterX = this->searchCenter.first;
    data.searchCenterY = this->searchCenter.second;
    data.searchPathIndex = this->searchPathIndex;
    return data;
}

void HardAI::restoreState(const AIStateData& data, Map* map) {
    this->currentState = (State)data.state;
    this->lastKnownPosition = {data.lastKnownPosX, data.lastKnownPosY};
    this->lastPlayerPosition = {data.lastPlayerPosX, data.lastPlayerPosY};
    this->turnsNotSeenCounter = data.turnsNotSeenCounter;
    this->searchCenter = {data.searchCenterX, data.searchCenterY};
    this->searchPathIndex = data.searchPathIndex;

    if (this->currentState == SEARCH && map) {
        generateSpiralSearch(this->searchCenter, map);
    }
}