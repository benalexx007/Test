#include "game.h"
#include "core/audio.h"
#include "screens/start.h"
#include "ai/mummy_ai.h"
#include <cmath>
static const int MAX_STAGE = 3;

void Game::init(char stage, Difficulty difficulty)
{
    currentStage = stage;
    currentDifficulty = difficulty;
    gameState = GameState::Playing;
    turn = 0;
    mummyStepsLeft = 0;
    settingsVisible = false;
    
    // Clear undo/redo stacks
    while (!undoStack.empty()) undoStack.pop();
    while (!redoStack.empty()) redoStack.pop();
    
    // Chỉ init SDL nếu chưa có window
    if (!renderer)
        renderer = SDL_CreateRenderer(window, NULL);
    
    // Khởi tạo User (giống như trong Start)
    user.read();
    user.Init();

    // background manager
    background = new Background(renderer);
    background->load(stage);

    map = new Map(renderer, stage);
    map->loadFromFile("assets/maps/level" + std::string(1, stage) + ".txt");
    int tileSize = map->getTileSize();
    int mapPxW = tileSize * map->getCols();
    int mapPxH = tileSize * map->getRows();
    offsetX = (winW - mapPxW) * 95 / 100;
    offsetY = (winH - mapPxH) / 2;

    ingamePanel = new IngamePanel(renderer);
    ingamePanel->create(renderer, 0, 0, 0, 0);
    ingamePanel->initForStage(this, winW, mapPxW, winH, mapPxH);

    int expX = 1, expY = 1;  // default fallback
    int mummyX = 5, mummyY = 5;  // default fallback
    map->getExplorerPosition(expX, expY);
    map->getMummyPosition(mummyX, mummyY);
    
    explorer = new Explorer(renderer, expX, expY, tileSize, stage);
    mummy = new Mummy(renderer, mummyX, mummyY, tileSize, stage);
    
    // Create and set AI based on difficulty
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

    // Save initial state
    initialState = getCurrentState();
    
    isRunning = true;
}

void Game::handleEvents()
{
    SDL_Event e;
    int curW = winW;
    int curH = winH;
    while (SDL_PollEvent(&e))
    {
        if (gameState == GameState::TheEnd) {
            if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN ||
                e.type == SDL_EVENT_KEY_DOWN ||
                e.type == SDL_EVENT_QUIT) {

                // Tắt game hiện tại
                isRunning = false;
                cleanup();   // đóng cửa sổ game, giải phóng tài nguyên

                // Mở lại Start menu
                Start start;
                start.run(window);
                return;      // Thoát hàm handleEvents
            }
            // Nếu không có input trên thì bỏ qua event
            continue;
        }
        if (e.type == SDL_EVENT_QUIT)
            isRunning = false;

        // forward input to UI panels so buttons get clicks
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

        if (!panelActive && turn == 0) {
            // Save state before explorer moves (only if explorer is at rest and about to move)
            if (e.type == SDL_EVENT_KEY_DOWN && explorer && explorer->isAtRest()) {
                // Check if this is a movement key
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

void Game::update()
{
    // tween vị trí mỗi frame
    explorer->updatePosition();
    mummy->updatePosition();

    // Nếu đang là lượt người chơi và người chơi vừa đi xong → bắt đầu lượt mummy (2 bước)
    if (turn == 0 && explorer->hasMoved())
    {
        turn = 1;
        mummyStepsLeft = 2;
        explorer->resetMoveFlag();
    }

    // Lượt của mummy: đi từng bước khi đã đứng tâm ô
    if (turn == 1)
    {
        if (mummyStepsLeft > 0 && mummy->isAtRest())
        {
            mummy->moveOneStep(map, explorer->getX(), explorer->getY());
            mummyStepsLeft--;
        }
        // Khi đã đi đủ 2 bước và dừng lại, trả lượt về cho người chơi
        if (mummyStepsLeft == 0 && mummy->isAtRest())
        {
            turn = 0;
        }
    }
    if (gameState == GameState::Playing && explorer && map) {
        if (map->isExit(explorer->getX(), explorer->getY())) {
    
            // Nếu đang ở màn tối đa (3) → chuyển sang màn hình THE END
                    // Nếu đang ở màn tối đa (3) → chuyển sang màn hình THE END
        if ((currentStage - '0') >= MAX_STAGE) {
            gameState = GameState::TheEnd;

            // Tạo text "THE END" bằng class Text có sẵn trong project
            SDL_Color color = {255, 255, 255, 255};
            if (theEndText.create(renderer, "assets/font.ttf", 120, "THE END", color)) {
                int x = (winW - theEndText.getWidth()) / 2;
                int y = (winH - theEndText.getHeight()) / 2;
                theEndText.setPosition(x, y);
            }
        } else {
            // Ngược lại: xử lý thắng bình thường, hiện VictoryPanel + nút Next
            gameState = GameState::Victory;
            if (currentStage >= '1' && currentStage <= '2') {
                user.updateStage(currentStage + 1);
            }
                if (!victoryPanel) {
                    victoryPanel = new VictoryPanel(renderer);
                    if (victoryPanel->init(1750, 900, [this]() {
                        // Next level callback - restart game với stage mới
                        char nextStage = currentStage + 1;
                        std::cerr << "Loading next level: " << nextStage << "\n";  // Debug
                        cleanupForRestart();
                        init(nextStage);
                        std::cerr << "Next level loaded\n";  // Debug
                    })) {
                        int px = (winW - victoryPanel->getWidth()) / 2;
                        int py = (winH - victoryPanel->getHeight()) / 2;
                        victoryPanel->setPosition(px, py);
                    }
                }
            }
        }
    }

    // Check collision: Mummy với Explorer (Lost)
    if (gameState == GameState::Playing && explorer && mummy) {
        if (explorer->getX() == mummy->getX() && explorer->getY() == mummy->getY()) {
            gameState = GameState::Lost;
            // Tạo lost panel
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

void Game::render()
{
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    // ===== THÊM KHỐI XỬ LÝ THE END TẠI ĐÂY =====
    if (gameState == GameState::TheEnd) {
        theEndText.render();
        SDL_RenderPresent(renderer);
        return; 
    }
    // ===== HẾT KHỐI THE END =====

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

void Game::cleanup()
{
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
}
void Game::cleanupForRestart()
{
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
    // Reset game state
    gameState = GameState::Playing;  // Thêm dòng này
    turn = 0;  // Thêm dòng này
    mummyStepsLeft = 0;  // Thêm dòng này
    settingsVisible = false;  // Thêm dòng này
    
    // KHÔNG destroy window và renderer - giữ lại để restart
    // KHÔNG gọi SDL_Quit(), TTF_Quit(), và không xóa g_audioInstance
}
void Game::run(char stage, SDL_Window *win)
{
    window = win;
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
        SDL_Delay(16); // ~60 FPS
    }
    cleanup();
}

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
    // Truyền isInGame = true và callback để thoát game
    if (!settingsPanel->init(&user, panelW, panelH, 
        [this]() {
            // Callback cho RETURN: đóng panel và tiếp tục chơi
            settingsVisible = false;
            if (settingsPanel) { 
                settingsPanel->cleanup(); 
                delete settingsPanel; 
                settingsPanel = nullptr; 
            }
        },
        true, // isInGame = true
        [this]() {
            cleanup();
            Start start;
            start.run(window);
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

// Undo/Redo/Reset implementation
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
    return snapshot;
}

void Game::saveState() {
    if (gameState != GameState::Playing) return; // Don't save if game is over
    GameStateSnapshot snapshot = getCurrentState();
    undoStack.push(snapshot);
    // Clear redo stack when new action is performed
    while (!redoStack.empty()) redoStack.pop();
}

void Game::restoreState(const GameStateSnapshot& snapshot) {
    if (!explorer || !mummy) return;
    
    // Restore positions
    explorer->moveTo(snapshot.explorerX, snapshot.explorerY);
    mummy->moveTo(snapshot.mummyX, snapshot.mummyY);
    
    // Restore game state
    turn = snapshot.turn;
    mummyStepsLeft = snapshot.mummyStepsLeft;
    gameState = snapshot.gameState;
    
    // Reset move flag if it's explorer's turn
    if (turn == 0) {
        explorer->resetMoveFlag();
    }
    
    // Clean up panels if game state changed
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

void Game::undo() {
    if (undoStack.empty() || gameState != GameState::Playing) return;
    
    // Save current state to redo stack
    redoStack.push(getCurrentState());
    
    // Restore previous state
    GameStateSnapshot snapshot = undoStack.top();
    undoStack.pop();
    restoreState(snapshot);
}

void Game::redo() {
    if (redoStack.empty() || gameState != GameState::Playing) return;
    
    // Save current state to undo stack
    undoStack.push(getCurrentState());
    
    // Restore next state
    GameStateSnapshot snapshot = redoStack.top();
    redoStack.pop();
    restoreState(snapshot);
}

void Game::reset() {
    if (gameState == GameState::TheEnd) return; // Don't reset if at THE END
    
    // Clear stacks
    while (!undoStack.empty()) undoStack.pop();
    while (!redoStack.empty()) redoStack.pop();
    
    // Restore initial state
    restoreState(initialState);
}