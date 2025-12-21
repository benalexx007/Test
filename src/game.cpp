#include "game.h"
#include "core/audio.h"
#include "screens/start.h"
#include "ai/mummy_ai.h"
#include <cmath>

// Maximum stage number. This constant controls when the game should
// display the final "THE END" screen instead of a normal victory
// panel. It is defined here to centralize gameplay constants.
static const int MAX_STAGE = 3;

// Initialize the game for a given stage and difficulty. This method
// is responsible for allocating resources, creating entities, setting
// up AI, and preparing UI panels for the level.
void Game::init(char stage, Difficulty difficulty)
{
    // Record stage and difficulty metadata, and reset runtime flags.
    currentStage = stage;
    currentDifficulty = difficulty;
    gameState = GameState::Playing;
    turn = 0;
    mummyStepsLeft = 0;
    settingsVisible = false;

    // Clear undo/redo history when starting a new level.
    while (!undoStack.empty()) undoStack.pop();
    while (!redoStack.empty()) redoStack.pop();

    // Create a renderer if one has not been created yet. The renderer
    // is used for all texture drawing and is associated with the
    // existing SDL_Window provided by the caller.
    if (!renderer)
        renderer = SDL_CreateRenderer(window, NULL);

    // Load persistent user data and apply any stored settings.
    user.read();
    user.Init();

    // Initialize background manager and load the assets for the stage.
    background = new Background(renderer);
    background->load(stage);

    // Create and load the map for the requested stage. Map coordinates
    // are used to compute pixel offsets for centering within the window.
    map = new Map(renderer, stage);
    map->loadFromFile("assets/maps/level" + std::string(1, stage) + ".txt");
    int tileSize = map->getTileSize();
    int mapPxW = tileSize * map->getCols();
    int mapPxH = tileSize * map->getRows();
    offsetX = (winW - mapPxW) * 95 / 100;
    offsetY = (winH - mapPxH) / 2;

    // Initialize the in-game UI panel and set it up for the current stage.
    ingamePanel = new IngamePanel(renderer);
    ingamePanel->create(renderer, 0, 0, 0, 0);
    ingamePanel->initForStage(this, winW, mapPxW, winH, mapPxH);

    // Query the map for entity starting positions. Fallback values are
    // provided to guard against malformed level data.
    int expX = 1, expY = 1;  // default fallback
    int mummyX = 5, mummyY = 5;  // default fallback
    map->getExplorerPosition(expX, expY);
    map->getMummyPosition(mummyX, mummyY);

    // Construct entity objects with their initial positions and tile sizes.
    explorer = new Explorer(renderer, expX, expY, tileSize, stage);
    mummy = new Mummy(renderer, mummyX, mummyY, tileSize, stage);

    // Instantiate a concrete AI implementation according to the
    // configured difficulty level. The `HardAI` requires the exit
    // coordinates; note the coordinate tuple ordering expected by
    // the `HardAI` constructor.
    IMummyAI* ai = nullptr;
    if (difficulty == Difficulty::Easy) {
        ai = new EasyAI();
    } else if (difficulty == Difficulty::Medium) {
        ai = new MediumAI();
    } else if (difficulty == Difficulty::Hard) {
        int exitX, exitY;
        map->getExitPosition(exitX, exitY);
        ai = new HardAI({exitY, exitX}); // HardAI uses (row, col) = (y, x)
    }
    if (ai) {
        mummy->setAI(ai);
    }

    // Record the initial state for future resets.
    initialState = getCurrentState();

    isRunning = true;
}

// Poll and dispatch system and UI events. This method forwards events
// to UI panels, performs window resizing logic, and forwards input to
// the explorer entity. It also handles level-exit and return-to-menu
// semantics when appropriate.
void Game::handleEvents()
{
    SDL_Event e;
    int curW = winW;
    int curH = winH;
    while (SDL_PollEvent(&e))
    {
        // If the game has reached the final end state, any input or quit
        // event returns the user to the start menu. This preserves the
        // expected behaviour of a final credits/end screen.
        if (gameState == GameState::TheEnd) {
            if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN ||
                e.type == SDL_EVENT_KEY_DOWN ||
                e.type == SDL_EVENT_QUIT) {

                // Stop the level and release resources and return to caller
                // (the caller will typically re-show the Start screen).
                isRunning = false;
                return;      // Exit event processing to avoid further handling.
            }
            // If no relevant input, skip remaining event handling.
            continue;
        }

        // A quit event during gameplay returns the user to the Start screen
        // rather than terminating the entire application. This design helps
        // preserve the global window/audio context managed at higher level.
        if (e.type == SDL_EVENT_QUIT) {
            // Treat quit as return-to-menu for the running level.
            isRunning = false;
            return;
        }

        // Forward input to active UI panels so buttons and dialogs can
        // respond to user interaction. The `panelActive` flag prevents
        // the same event from being processed twice (e.g., both panel and
        // explorer).
        bool panelActive = false;
        if (settingsVisible && settingsPanel) {
            settingsPanel->handleEvent(e);
            panelActive = true;
        }
        if (gameState == GameState::Victory && victoryPanel) {
            victoryPanel->handleEvent(e);
            panelActive = true;
        }
        if (gameState == GameState::Lost && lostPanel) {
            lostPanel->handleEvent(e);
            panelActive = true;
        }
        if (!panelActive && ingamePanel) {
            ingamePanel->handleEvent(e);
        }

        // Handle window resize events. The code preserves a fixed aspect
        // ratio by adjusting width/height accordingly and applies a render
        // scale to keep visual elements proportionate.
        if (e.type == SDL_EVENT_WINDOW_RESIZED)
        {
            int w = e.window.data1;
            int h = e.window.data2;
            if (w != curW)
            {
                h = static_cast<int>(w / windowRatio);
            }
            else if (h != curH)
            {
                w = static_cast<int>(h * windowRatio);
            }
            SDL_SetWindowSize(window, w, h);
            float scale = static_cast<float>(w) / static_cast<float>(winW);
            SDL_SetRenderScale(renderer, scale, scale);
            curW = w;
            curH = h;
        }

        // Only allow explorer input when no UI panel has captured the
        // event and it is the explorer's turn. Additionally, detect
        // movement keys in order to save the state prior to movement so
        // undo semantics can revert the action.
        if (!panelActive && turn == 0) {
            // Save state before explorer moves (only if explorer is at rest and about to move)
            if (e.type == SDL_EVENT_KEY_DOWN && explorer && explorer->isAtRest()) {
                // Check whether the key is a movement key; if so, record
                // the pre-move state to the undo stack.
                bool isMovementKey = (e.key.key == SDLK_UP || e.key.key == SDLK_DOWN || 
                                     e.key.key == SDLK_LEFT || e.key.key == SDLK_RIGHT);
                if (isMovementKey) {
                    saveState();
                }
            }
            explorer->handleInput(e, map);
        }
    }
}

// Update game logic once per frame. This includes interpolating entity
// positions for smooth rendering, resolving turn transitions, invoking
// mummy AI steps, and handling win/loss conditions.
void Game::update()
{
    // Update interpolated positions for smooth movement animation.
    explorer->updatePosition();
    mummy->updatePosition();

    // If the explorer has completed a move during the explorer's turn,
    // transfer control to the mummy and permit two discrete mummy steps.
    if (turn == 0 && explorer->hasMoved())
    {
        turn = 1;
        mummyStepsLeft = 2;
        explorer->resetMoveFlag();
        // Intentionally perform only one AI step per update so that each
        // mummy movement can be animated independently; the loop above
        // orchestrates per-step execution.
    }

    // Mummy's turn: execute up to `mummyStepsLeft` discrete steps, each
    // of which is invoked only when the mummy is at rest (prevents step
    // overlap and preserves animation timing).
    if (turn == 1)
    {
        if (mummyStepsLeft > 0 && mummy->isAtRest())
        {
            mummy->moveOneStep(map, explorer->getX(), explorer->getY());
            mummyStepsLeft--;
        }
        // When the mummy has exhausted its allotted steps and is at rest,
        // return the turn to the explorer.
        if (mummyStepsLeft == 0 && mummy->isAtRest())
        {
            turn = 0;
        }
    }

    // Handle victory conditions: if the explorer reaches the map exit,
    // either progress to the next stage (showing a victory panel) or,
    // if the maximum stage is reached, display the final "THE END".
    if (gameState == GameState::Playing && explorer && map) {
        if (map->isExit(explorer->getX(), explorer->getY())) {

            // If the player completed the final stage, transition to
            // the terminal screen; otherwise show the victory panel with
            // a callback to load the subsequent stage.
            if ((currentStage - '0') >= MAX_STAGE) {
                gameState = GameState::TheEnd;

                // Create a large central text object to render "THE END".
                SDL_Color color = {255, 255, 255, 255};
                if (theEndText.create(renderer, "assets/font.ttf", 120, "THE END", color)) {
                    int x = (winW - theEndText.getWidth()) / 2;
                    int y = (winH - theEndText.getHeight()) / 2;
                    theEndText.setPosition(x, y);
                }
            } else {
                // Normal victory: award level progress and instantiate a
                // `VictoryPanel` if one does not already exist. The panel's
                // callback advances to the next stage by reinitializing
                // the game state for the subsequent level.
                gameState = GameState::Victory;
                if (currentStage >= '1' && currentStage <= '2') {
                    user.updateStage(currentStage + 1);
                }
                if (!victoryPanel) {
                    victoryPanel = new VictoryPanel(renderer);
                    if (victoryPanel->init(1750, 900, [this]() {
                        // Next level callback - restart game with new stage
                        char nextStage = currentStage + 1;
                        std::cerr << "Loading next level: " << nextStage << "\n";  // Debug output
                        cleanupForRestart();
                        // Map stage to difficulty explicitly so AI resets correctly
                        Difficulty nextDiff = Difficulty::Easy;
                        if (nextStage == '2') nextDiff = Difficulty::Medium;
                        else if (nextStage == '3') nextDiff = Difficulty::Hard;
                        init(nextStage, nextDiff);
                        std::cerr << "Next level loaded\n";  // Debug output
                    })) {
                        int px = (winW - victoryPanel->getWidth()) / 2;
                        int py = (winH - victoryPanel->getHeight()) / 2;
                        victoryPanel->setPosition(px, py);
                    }
                }
            }
        }
    }

    // Check collision: if the mummy occupies the explorer's tile
    // then the player loses and a lost dialog is created with a
    // callback that restarts the current level.
    if (gameState == GameState::Playing && explorer && mummy) {
        if (explorer->getX() == mummy->getX() && explorer->getY() == mummy->getY()) {
            gameState = GameState::Lost;
            if (!lostPanel) {
                lostPanel = new LostPanel(renderer);
                if (lostPanel->init(1750, 900, [this]() {
                    // Play again callback - restart current level
                    cleanupForRestart();
                    init(currentStage, currentDifficulty);
                })) {
                    int px = (winW - lostPanel->getWidth()) / 2;
                    int py = (winH - lostPanel->getHeight()) / 2;
                    lostPanel->setPosition(px, py);
                }
            }
        }
    }
}

// Render the current frame. This method draws the background, map,
// entities, and any active UI panels. The final End state is handled
// specially by rendering the centered "THE END" message.
void Game::render()
{
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    // If the game is in the terminal THE END state, render only the
    // end text and present the frame to avoid drawing the regular UI.
    if (gameState == GameState::TheEnd) {
        theEndText.render();
        SDL_RenderPresent(renderer);
        return; 
    }

    // Standard rendering pipeline for a playing level.
    if (background) background->render(winW, winH);
    map->render(offsetX, offsetY);
    explorer->render(offsetX, offsetY);
    mummy->render(offsetX, offsetY);
    if (ingamePanel) ingamePanel->render();
    if (settingsVisible && settingsPanel) settingsPanel->render();
    if (gameState == GameState::Victory && victoryPanel) victoryPanel->render();
    if (gameState == GameState::Lost && lostPanel) lostPanel->render();

    SDL_RenderPresent(renderer);
}

// Release all resources owned by the game instance. This includes
// panels, entities, the map, and the renderer. The method leaves the
// application in a state suitable for returning control to the Start
// screen or terminating the program.
void Game::cleanup()
{
    std::cerr << "Game: cleanup() start\n";
    if (background) {
        background->cleanup();
        delete background;
        background = nullptr;
    }

    if (ingamePanel) {
        ingamePanel->cleanup();
        delete ingamePanel;
        ingamePanel = nullptr;
    }
    if (settingsPanel) {
        settingsPanel->cleanup();
        delete settingsPanel;
        settingsPanel = nullptr;
    }
    if (victoryPanel) {
        victoryPanel->cleanup();
        delete victoryPanel;
        victoryPanel = nullptr;
    }
    if (lostPanel) {
        lostPanel->cleanup();
        delete lostPanel;
        lostPanel = nullptr;
    }
    
    delete map;
    map = nullptr;

    delete explorer;
    explorer = nullptr;

    delete mummy;
    mummy = nullptr;

    SDL_DestroyRenderer(renderer);
    renderer = nullptr;

    theEndText.cleanup();

    isRunning = false;
    std::cerr << "Game: cleanup() end\n";
}

// Partial cleanup performed prior to restarting or switching levels.
// This function intentionally preserves the main window and global
// subsystems (such as audio) so that the Start menu may be displayed
// without a full application restart.
void Game::cleanupForRestart()
{
    std::cerr << "Game: cleanupForRestart() start\n";
    if (background) {
        background->cleanup();
        delete background;
        background = nullptr;
    }
    if (ingamePanel) {
        ingamePanel->cleanup();
        delete ingamePanel;
        ingamePanel = nullptr;
    }
    if (settingsPanel) {
        settingsPanel->cleanup();
        delete settingsPanel;
        settingsPanel = nullptr;
    }
    if (victoryPanel) {
        victoryPanel->cleanup();
        delete victoryPanel;
        victoryPanel = nullptr;
    }
    if (lostPanel) {
        lostPanel->cleanup();
        delete lostPanel;
        lostPanel = nullptr;
    }
    delete map;
    map = nullptr;
    delete explorer;
    explorer = nullptr;
    delete mummy;
    mummy = nullptr;
    theEndText.cleanup();
    // Destroy renderer so the Start screen can create its own renderer
    if (renderer) {
        SDL_DestroyRenderer(renderer);
        renderer = nullptr;
    }
    // Reset volatile game state variables so the next init() call will
    // start from a consistent baseline.
    gameState = GameState::Playing;
    turn = 0;
    mummyStepsLeft = 0;
    settingsVisible = false;

    // Do NOT destroy the window or call global subsystem shutdown
    // functions here; those responsibilities belong to top-level
    // application code (e.g., main.cpp).

    std::cerr << "Game: cleanupForRestart() end\n";
}

// Run the primary game loop for a specified stage using an existing
// SDL_Window. The loop polls events, updates the game logic, and
// renders frames until the `isRunning` flag becomes false.
void Game::run(char stage, SDL_Window *win)
{
    window = win;
    std::cerr << "Game: entering run for stage " << stage << "\n";
    // Set difficulty based on stage: 1=Easy, 2=Medium, 3=Hard
    Difficulty difficulty = Difficulty::Easy;
    if (stage == '2') {
        difficulty = Difficulty::Medium;
    } else if (stage == '3') {
        difficulty = Difficulty::Hard;
    }
    init(stage, difficulty);
    while (isRunning)
    {
        handleEvents();
        update();
        render();
        SDL_Delay(16); // ~60 FPS target frame timing
    }
    cleanup();
    std::cerr << "Game: run exiting for stage " << stage << "\n";
}

// Toggle the settings panel visibility. When opening, initialize the
// settings panel and register callbacks for 'return' and 'exit to menu'
// actions. When closing, perform the appropriate cleanup.
void Game::toggleSettings() {
    if (settingsVisible) {
        settingsVisible = false;
        if (settingsPanel) { 
            settingsPanel->cleanup(); 
            delete settingsPanel; 
            settingsPanel = nullptr; 
        }
        return;
    }
    settingsPanel = new SettingsPanel(renderer);
    const int panelW = 1750, panelH = 900;
    // Initialize the settings panel with callbacks for returning to
    // the level and for exiting to the Start screen. Passing `this`
    // allows callback functions to invoke undo/redo/reset through
    // the Game instance.
    if (!settingsPanel->init(&user, panelW, panelH, 
        [this]() {
            // Callback for RETURN: close the settings panel and resume play
            settingsVisible = false;
            if (settingsPanel) { 
                settingsPanel->cleanup(); 
                delete settingsPanel; 
                settingsPanel = nullptr; 
            }
        },
        true, // isInGame = true
        [this]() {
            // Callback for EXIT TO MENU: stop the current game and return
            // to the caller (Start) which will re-show the main menu.
            std::cerr << "Game: EXIT TO MENU requested\n";
            isRunning = false;
            // Do not create a new Start instance here; allow the
            // outer caller to handle showing the Start screen.
            return;
        },
        this)) { // Pass Game* pointer for undo/redo/reset
        delete settingsPanel;
        settingsPanel = nullptr;
        return;
    }
    int px = (winW - panelW) / 2;
    int py = (winH - panelH) / 2;
    settingsPanel->setPosition(px, py);
    settingsVisible = true;
}

// --- Undo / Redo / Reset Implementation ---

// Produce a snapshot representing the current logical game state.
GameStateSnapshot Game::getCurrentState() const {
    GameStateSnapshot snapshot;
    if (explorer) {
        snapshot.explorerX = explorer->getX();
        snapshot.explorerY = explorer->getY();
    }
    if (mummy) {
        snapshot.mummyX = mummy->getX();
        snapshot.mummyY = mummy->getY();
    }
    snapshot.turn = turn;
    snapshot.mummyStepsLeft = mummyStepsLeft;
    snapshot.gameState = gameState;
    snapshot.stage = currentStage;
    snapshot.difficulty = currentDifficulty;
    return snapshot;
}

// Save the current state onto the undo stack. The redo stack is
// cleared as a new action invalidates the forward history.
void Game::saveState() {
    if (gameState != GameState::Playing) return; // Do not save terminal states
    GameStateSnapshot snapshot = getCurrentState();
    if (mummy) {
        snapshot.aiState = mummy->getAIState();
    }
    undoStack.push(snapshot);
    while (!redoStack.empty()) redoStack.pop();
}

// Restore a previously captured snapshot. This method restores entity
// positions, turn metadata, game state, and reconfigures the mummy AI
// to match the snapshot's difficulty setting so that undo/redo fully
// reverts algorithmic behaviour as well as positions.
void Game::restoreState(const GameStateSnapshot& snapshot) {
    if (!explorer || !mummy) return;
    
    // Restore positions of entities.
    explorer->moveTo(snapshot.explorerX, snapshot.explorerY);
    mummy->moveTo(snapshot.mummyX, snapshot.mummyY);
    
    // Restore control flow and game flags.
    turn = snapshot.turn;
    mummyStepsLeft = snapshot.mummyStepsLeft;
    gameState = snapshot.gameState;

    // Restore stage/difficulty and reinitialize mummy AI accordingly.
    currentStage = snapshot.stage;
    currentDifficulty = snapshot.difficulty;
    mummy->setAI(nullptr); // Ensure existing AI is removed safely
    IMummyAI* ai = nullptr;
    if (currentDifficulty == Difficulty::Easy) {
        ai = new EasyAI();
    } else if (currentDifficulty == Difficulty::Medium) {
        ai = new MediumAI();
    } else if (currentDifficulty == Difficulty::Hard) {
        int exitX, exitY;
        if (map) map->getExitPosition(exitX, exitY);
        ai = new HardAI({exitY, exitX});
    }
    if (ai) mummy->setAI(ai);

    if (mummy) {
        mummy->restoreAIState(snapshot.aiState, map);
    }
    
    // If it is the explorer's turn, ensure movement flags are reset
    // so that input handling and animation behave consistently.
    if (turn == 0) {
        explorer->resetMoveFlag();
    }
    
    // If we restored a non-terminal playing state, remove any modal
    // panels that might have been present (victory/lost dialogs).
    if (snapshot.gameState == GameState::Playing) {
        if (victoryPanel) {
            victoryPanel->cleanup();
            delete victoryPanel;
            victoryPanel = nullptr;
        }
        if (lostPanel) {
            lostPanel->cleanup();
            delete lostPanel;
            lostPanel = nullptr;
        }
    }
}

// Revert to the previous state by popping the undo stack and
// pushing the current state onto the redo stack to enable forward
// navigation.
void Game::undo() {
    if (undoStack.empty() || gameState != GameState::Playing) return;
    redoStack.push(getCurrentState());
    GameStateSnapshot snapshot = undoStack.top();
    undoStack.pop();
    restoreState(snapshot);
}

// Advance to a previously undone state by consuming the redo stack
// and saving the current state onto the undo stack.
void Game::redo() {
    if (redoStack.empty() || gameState != GameState::Playing) return;
    undoStack.push(getCurrentState());
    GameStateSnapshot snapshot = redoStack.top();
    redoStack.pop();
    restoreState(snapshot);
}

// Reset the level to its initially recorded state. This clears the
// undo/redo history and invokes `restoreState` with the snapshot
// captured at the time of initialization.
void Game::reset() {
    if (gameState == GameState::TheEnd) return; // Protect terminal state
    while (!undoStack.empty()) undoStack.pop();
    while (!redoStack.empty()) redoStack.pop();
    restoreState(initialState);
}