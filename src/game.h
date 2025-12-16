#pragma once
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <iostream>
#include <cstring>  
#include "ingame/map.h"
#include "ingame/background.h"
#include "ingame/panel.h"
#include "entities/explorer.h"
#include "entities/mummy.h"
#include "text.h"
#include "functions.h"
#include "user.h"
#include <vector>

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
    enum class GameState { Playing, Victory, Lost };
    GameState gameState = GameState::Playing;

    struct State {
        int expX, expY;
        int mumX, mumY;
    };

    // [Thêm] Hai ngăn xếp để lưu lịch sử
    std::vector<State> undoStack;
    std::vector<State> redoStack;

    // [Thêm] Hàm hỗ trợ
    void saveState();        // Lưu trạng thái hiện tại vào undoStack
    void performUndo();      // Thực hiện Undo
    void performRedo();      // Thực hiện Redo
    void restoreState(const State& state); // Khôi phục vị trí từ State
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

    void init(const char stage);
    void handleEvents();
    void update();
    void render();
    void cleanup();
    void cleanupForRestart();
    void run(const char stage);
    void toggleSettings();
};