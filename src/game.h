#pragma once
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <iostream>
#include <cstring>
#include <vector>
#include <stack>
#include "ingame/map.h"
#include "ingame/background.h"
#include "ingame/panel.h"
#include "entities/explorer.h"
#include "entities/mummy.h"
#include "ui/text.h"
#include "core/functions.h"
#include "user.h"

enum class GameState { Playing, Victory, Lost, TheEnd };
enum class Difficulty { Easy, Medium, Hard };

struct GameStateSnapshot {
    int explorerX;
    int explorerY;
    int mummyX;
    int mummyY;
    int turn;
    int mummyStepsLeft;
    GameState gameState;
    
    GameStateSnapshot() : explorerX(0), explorerY(0), mummyX(0), mummyY(0), turn(0), mummyStepsLeft(0), gameState(GameState::Playing) {}
};

class Game {
private:
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    Background* background = nullptr;
    bool isRunning = false;
    int turn = 0; // 0 = Explorer, 1 = Mummy
    int mummyStepsLeft = 0;
    int winW = 1920;
    int winH = 991;
    float windowRatio = 1920.0/991.0;
    int offsetX = 0;
    int offsetY = 0;
    User user;
    char currentStage;
    Difficulty currentDifficulty = Difficulty::Easy;
    GameState gameState = GameState::Playing;

    // Undo/Redo system
    std::stack<GameStateSnapshot> undoStack;
    std::stack<GameStateSnapshot> redoStack;
    GameStateSnapshot initialState;

    // Text hiển thị "THE END"
    Text theEndText;
public:
    Map* map = nullptr;
    Explorer* explorer = nullptr;
    Mummy* mummy = nullptr;
    // in-game UI panel on the right side
    IngamePanel* ingamePanel = nullptr;
    SettingsPanel* settingsPanel = nullptr;
    VictoryPanel* victoryPanel = nullptr;
    LostPanel* lostPanel = nullptr;
    bool settingsVisible = false;

    void init(const char stage, Difficulty difficulty = Difficulty::Easy);
    void handleEvents();
    void update();
    void render();
    void cleanup();
    void cleanupForRestart();
    void run(const char stage, SDL_Window* SDL_Window);
    void toggleSettings();
    
    // Undo/Redo/Reset methods
    void saveState();
    void restoreState(const GameStateSnapshot& snapshot);
    void undo();
    void redo();
    void reset();
    GameStateSnapshot getCurrentState() const;
    
    // Friend functions for button callbacks
    friend void undo(Game* game);
    friend void redo(Game* game);
    friend void reset(Game* game);
};