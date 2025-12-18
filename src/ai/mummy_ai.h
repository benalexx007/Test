#pragma once
#include <utility>
#include <vector>
#include <string>

// Forward declarations
class Map;

// Interface for AI Strategy Pattern
class IMummyAI {
public:
    virtual ~IMummyAI() {}
    virtual void move(std::pair<int, int>& mummyPosition,
                      const std::pair<int, int>& playerPosition,
                      Map* map) = 0;
};

// Easy AI Implementation
// Behavior: Random movement mixed with simple direct chasing
class EasyAI : public IMummyAI {
private:
    int slope = 5;
    int chaseDistance = 7;
    
    bool isValidPosition(int row, int col, Map* map);

public:
    void move(std::pair<int, int>& mummyPosition,
              const std::pair<int, int>& playerPosition,
              Map* map) override;
};

// Medium AI Implementation
// Behavior: Greedy movement with BFS for stuck recovery
class MediumAI : public IMummyAI {
private:
    int stuckCounter = 0;
    int bfsModeTurnsRemaining = 0;
    std::pair<int, int> lastPosition = {-1, -1};
    
    bool isValidPosition(int row, int col, Map* map);
    std::pair<int, int> getNextBfsMove(const std::pair<int, int>& start,
                                       const std::pair<int, int>& goal,
                                       Map* map);

public:
    void move(std::pair<int, int>& mummyPosition,
              const std::pair<int, int>& playerPosition,
              Map* map) override;
};

// Hard AI Implementation
// Behavior: State Machine (Hunt, Search, Intercept, Patrol)
class HardAI : public IMummyAI {
private:
    enum State {
        HUNT,
        SEARCH,
        INTERCEPT,
        PATROL
    };
    
    State currentState = PATROL;
    std::pair<int, int> lastKnownPosition = {-1, -1};
    std::pair<int, int> lastPlayerPosition = {-1, -1};
    int turnsNotSeenCounter = 0;
    
    const int searchRadius = 3;
    std::pair<int, int> exitPosition;
    
    // Search Variables
    std::pair<int, int> searchCenter = {-1, -1};
    std::vector<std::pair<int, int>> searchPath;
    int searchPathIndex = 0;
    
    bool isValidPosition(int row, int col, Map* map);
    bool hasLineOfSight(std::pair<int, int> mummyPosition, 
                        std::pair<int, int> playerPosition, 
                        Map* map);
    int getBfsDistance(const std::pair<int, int>& start, 
                      const std::pair<int, int>& goal, 
                      Map* map);
    std::pair<int, int> getNextBfsMove(const std::pair<int, int>& start,
                                      const std::pair<int, int>& goal,
                                      Map* map);
    std::pair<int, int> predictPlayerPosition(const std::pair<int, int>& playerPosition);
    void generateSpiralSearch(const std::pair<int, int>& center, Map* map);
    std::pair<int, int> findInterceptPoint(const std::pair<int, int>& mummyPosition,
                                          const std::pair<int, int>& suspectedPlayerPosition,
                                          Map* map);

public:
    HardAI(std::pair<int, int> exitPos);
    void move(std::pair<int, int>& mummyPosition,
              const std::pair<int, int>& playerPosition,
              Map* map) override;
};

