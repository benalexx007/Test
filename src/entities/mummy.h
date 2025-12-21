// The Mummy class represents the adversary entity. It may be
// controlled by a pluggable AI implementation (`IMummyAI`) or fall
// back to a simple greedy chase algorithm. The class provides
// facilities to perform discrete movement steps used by the game
// turn model.
#pragma once
#include "character.h"
#include "../ai/mummy_ai.h" // [Quan trọng] Include file này để hiểu AIStateData là gì

class IMummyAI;
class Map;

class Mummy : public Character {
private:
    IMummyAI* ai = nullptr; // Owned AI instance; setAI will delete previous AI
    
    // When an AI chooses to remain stationary, the game requires the
    // mummy to still perform two distinct steps for animation parity.
    // `forcedReturnPending` implements a two-step surrogate: the
    // mummy moves to a neighbor and then returns on the next step.
    bool forcedReturnPending = false;
    int forcedReturnCol = 0;
    int forcedReturnRow = 0;

public:
    Mummy(SDL_Renderer* renderer, int startX, int startY, int tileSize, char stage);
    ~Mummy();

    // Perform a single movement step towards the provided target
    // coordinates. This method consults the AI when available and
    // otherwise uses a simple heuristic-based chase.
    void moveOneStep(Map* map, int targetX, int targetY);
    
    // Convenience method that executes two consecutive steps.
    void chase(Map* map, int targetX, int targetY);
    
    void setAI(IMummyAI* aiInstance);
    IMummyAI* getAI() const { return ai; }

    AIStateData getAIState();
    
    void restoreAIState(const AIStateData& data, Map* map);
};