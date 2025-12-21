// AI strategies for the adversary (mummy). The AI subsystem is
// implemented using the Strategy Pattern: `IMummyAI` defines an
// interface and concrete classes provide progressively more
// sophisticated behaviours.
#pragma once
#include <utility>
#include <vector>
#include <string>

// Forward declarations
class Map;

// Interface for AI Strategy Pattern. The `move` method is expected to
// modify `mummyPosition` to the next desired location (row,col). Note
// that coordinates are expressed as (row, col) to match the Map's
// indexing convention.
struct AIStateData {
    int stuckCounter = 0;
    int bfsModeTurnsRemaining = 0;
    int lastPosX = -1;
    int lastPosY = -1;

    int state = 3;
    int lastKnownPosX = -1;
    int lastKnownPosY = -1;
    int lastPlayerPosX = -1;
    int lastPlayerPosY = -1;
    int turnsNotSeenCounter = 0;
    int searchCenterX = -1;
    int searchCenterY = -1;
    int searchPathIndex = 0;
    unsigned int rngSeed = 0;
};

class IMummyAI {
public:
    virtual ~IMummyAI() {}
    virtual void move(std::pair<int, int>& mummyPosition,
                      const std::pair<int, int>& playerPosition,
                      Map* map) = 0;
    virtual AIStateData getState() = 0;
    virtual void restoreState(const AIStateData& data, Map* map) = 0;
};

// Easy AI Implementation
// Behavior: probabilistic mixing of random movement and direct
// chasing. The AI will prefer to chase the player when sufficiently
// close, otherwise it may wander randomly. This results in an
// adversary that is forgiving for novice players.
class EasyAI : public IMummyAI {
private:
    int slope = 5;
    int chaseDistance = 7;

    unsigned int currentSeed = 12345;
    int myRand(); 
    
    bool isValidPosition(int row, int col, Map* map);

public:
    EasyAI();

    void move(std::pair<int, int>& mummyPosition,
              const std::pair<int, int>& playerPosition,
              Map* map) override;
    AIStateData getState() override;
    void restoreState(const AIStateData& data, Map* map) override;
};

// Medium AI Implementation
// Behavior: greedy chase augmented with BFS-based recovery when the
// agent becomes stuck or oscillates. This AI is intended to be
// moderately challenging while still computationally lightweight.
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
    AIStateData getState() override;
    void restoreState(const AIStateData& data, Map* map) override;
};

// Hard AI Implementation
// Behavior: a finite-state machine (HUNT, SEARCH, INTERCEPT, PATROL)
// incorporating BFS pathfinding, line-of-sight checks, predictive
// interception, and heuristic scoring. This AI is designed to be
// aggressive and to anticipate player movement when possible.
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
    
    // Variables used when executing spiral search patterns
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
    AIStateData getState() override;
    void restoreState(const AIStateData& data, Map* map) override;
};

