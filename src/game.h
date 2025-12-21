// Include guard to prevent multiple inclusion in a single translation unit.
#pragma once

// External libraries used for windowing, image loading, and font rendering.
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <iostream>
#include <cstring>
#include <vector>
#include <stack>

// Project subsystems that the Game class composes: map rendering,
// background management, UI panels, and entity definitions.
#include "ingame/map.h"
#include "ingame/background.h"
#include "ingame/panel.h"
#include "entities/explorer.h"
#include "entities/mummy.h"
#include "ui/text.h"
#include "ai/mummy_ai.h"
#include "core/functions.h"
#include "user.h"

// High-level enumeration representing the principal states of the
// level-based gameplay loop. These values are used to control
// update and rendering logic as well as UI transitions.
enum class GameState { Playing, Victory, Lost, TheEnd };

// Discrete difficulty tiers that determine the behaviour of the
// adversary AI. The implementation maps these levels to concrete
// AI strategies elsewhere in the codebase.
enum class Difficulty { Easy, Medium, Hard };

// A compact snapshot of the game state sufficient to implement
// undo/redo functionality. The snapshot records the positions of
// the two entities (explorer and mummy), the active turn, the
// remaining mummy steps in its current turn, the logical game
// state (e.g., Playing or Victory), and metadata such as the
// current stage and difficulty. This struct is intentionally
// lightweight to facilitate frequent push/pop operations on stacks.

struct GameStateSnapshot {
    int explorerX;
    int explorerY;
    int mummyX;
    int mummyY;
    int turn;
    int mummyStepsLeft;
    GameState gameState;
    char stage;
    Difficulty difficulty;
    
    AIStateData aiState;

    // Default constructor produces a neutral snapshot representing
    // an initial playing state at stage '1' with easy difficulty.
    GameStateSnapshot() : explorerX(0), explorerY(0), mummyX(0), mummyY(0), turn(0), mummyStepsLeft(0), gameState(GameState::Playing), stage('1'), difficulty(Difficulty::Easy) {}
};

// The `Game` class encapsulates the logic, state, and resource
// ownership for a single play session (a level). It coordinates
// entities, the map, UI panels, audio/visual resources, and provides
// undo/redo semantics for the player's actions.
class Game {
private:
    // Platform/windowing resources. `window` is not created here
    // (it is supplied by the caller), but `renderer` is created
    // when initializing the in-game view if necessary.
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;

    // Visual background manager used for parallax or decorative
    // elements behind the tile map.
    Background* background = nullptr;

    // Execution control flags and turn management. `isRunning`
    // indicates whether the in-level loop should continue.
    bool isRunning = false;
    // Turn: 0 indicates explorer's turn, 1 indicates mummy's turn.
    int turn = 0; // 0 = Explorer, 1 = Mummy
    // Number of discrete movement steps remaining for the mummy
    // within its current turn (typically 2 per explorer move).
    int mummyStepsLeft = 0;

    // Window dimensions and aspect ratio used for layout and
    // rendering calculations. These are used when centering the
    // map and positioning UI elements.
    int winW = 1920;
    int winH = 991;
    float windowRatio = 1920.0/991.0;
    int offsetX = 0;
    int offsetY = 0;

    // Persistent user profile object used to save progress.
    User user;
    // Current stage identifier (character) e.g., '1', '2', '3'.
    char currentStage;
    Difficulty currentDifficulty = Difficulty::Easy;
    GameState gameState = GameState::Playing;

    // Undo/redo stacks that store `GameStateSnapshot` values. The
    // `initialState` records the state to which a level reset will
    // revert.
    std::stack<GameStateSnapshot> undoStack;
    std::stack<GameStateSnapshot> redoStack;
    GameStateSnapshot initialState;

    // Text object used to render the final "THE END" message when
    // the player completes the final stage.
    Text theEndText;
public:
    // Pointers to dynamically allocated objects representing the
    // active map and entities. These pointers are owned by the
    // `Game` class and are cleaned up in `cleanup()`.
    Map* map = nullptr;
    Explorer* explorer = nullptr;
    Mummy* mummy = nullptr;
    // In-game UI panels (right-side control panel and dialogs).
    IngamePanel* ingamePanel = nullptr;
    SettingsPanel* settingsPanel = nullptr;
    VictoryPanel* victoryPanel = nullptr;
    LostPanel* lostPanel = nullptr;
    bool settingsVisible = false;
    // When true, the game should stop and perform cleanupForRestart
    // outside of UI callbacks to avoid deleting panels while they
    // are executing their event handlers.
    bool exitToMenuRequested = false;

    // Public API: lifecycle management and main loop methods.
    void init(const char stage, Difficulty difficulty = Difficulty::Easy);
    void handleEvents();
    void update();
    void render();
    void cleanup();
    void cleanupForRestart();
    void run(const char stage, SDL_Window* SDL_Window);
    void toggleSettings();
    
    // Undo/Redo/Reset methods: these implement time-travel-like
    // controls that allow the user to step backwards and forwards
    // through previously recorded game snapshots.
    void saveState();
    void restoreState(const GameStateSnapshot& snapshot);
    void undo();
    void redo();
    void reset();
    GameStateSnapshot getCurrentState() const;
    
    // Friend declarations allow global button-callback functions to
    // access private members when wiring up UI controls. These
    // functions are thin adapters that invoke the corresponding
    // member methods on a `Game` instance.
    friend void undo(Game* game);
    friend void redo(Game* game);
    friend void reset(Game* game);
};