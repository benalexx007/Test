#include "game.h"
#include "audio.h"
#include "start.h"
#include <cmath>

// Khai báo global audio instance
extern Audio* g_audioInstance;

void Game::init(char stage)
{
    currentStage = stage;
    gameState = GameState::Playing;
    turn = 0;  // Thêm dòng này
    mummyStepsLeft = 0;  // Thêm dòng này
    settingsVisible = false;  // Thêm dòng này
    
    // Chỉ init SDL nếu chưa có window
    if (!window) {
        SDL_Init(SDL_INIT_VIDEO);
        
        // Khởi tạo audio nếu chưa có (để nhạc nền tiếp tục phát)
        if (!g_audioInstance) {
            SDL_Init(SDL_INIT_AUDIO);
            g_audioInstance = new Audio();
            if (g_audioInstance->init()) {
                // Load và phát nhạc nền
                if (g_audioInstance->loadBackgroundMusic("assets/audio/background_music.wav")) {
                    g_audioInstance->playBackgroundMusic(true); // Loop
                } else {
                    std::cerr << "Failed to load background music\n";
                }
            }
        }
        
        // initialize SDL_ttf before any Text/TTF usage
        if (TTF_Init() == 0) {
            std::cerr << "TTF_Init failed: " << SDL_GetError() << "\n";
        }

        window = SDL_CreateWindow("Mê Cung Tây Du", winW, winH, SDL_WINDOW_RESIZABLE);
        renderer = SDL_CreateRenderer(window, NULL);
        SDL_MaximizeWindow(window);
    }
    
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

    isRunning = true;
}

void Game::handleEvents()
{
    SDL_Event e;
    int curW = winW;
    int curH = winH;
    while (SDL_PollEvent(&e))
    {
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

        if (!panelActive && turn == 0) // Chỉ xử lý khi đến lượt người chơi
        {
            // 1. Xử lý phím tắt Undo (Z) và Redo (R)
            if (e.type == SDL_EVENT_KEY_DOWN) {
                if (e.key.key == SDLK_Z) {
                    performUndo();
                    continue; // Bỏ qua xử lý di chuyển nếu đang undo
                }
                if (e.key.key == SDLK_R) {
                    performRedo();
                    continue; 
                }
            }

            // 2. Xử lý di chuyển và Lưu trạng thái
            // Lấy vị trí trước khi bấm phím
            int oldExpX = explorer->getX();
            int oldExpY = explorer->getY();
            int oldMumX = mummy->getX();
            int oldMumY = mummy->getY();

            // Cho phép explorer xử lý input
            explorer->handleInput(e, map);

            // Kiểm tra xem Explorer có thực sự di chuyển không?
            // Nếu tọa độ thay đổi, nghĩa là nước đi hợp lệ -> Lưu vào Undo Stack
            if (explorer->getX() != oldExpX || explorer->getY() != oldExpY) {
                // Tạo state cũ thủ công để lưu
                State oldState = { oldExpX, oldExpY, oldMumX, oldMumY };
                undoStack.push_back(oldState);
                
                // Khi có nước đi mới, phải xóa lịch sử Redo
                redoStack.clear();
            }
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
            gameState = GameState::Victory;
            // Tạo victory panel
            if (!victoryPanel) {
                victoryPanel = new VictoryPanel(renderer);
                if (victoryPanel->init(winW, winH, [this]() {
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

    // Check collision: Mummy với Explorer (Lost)
    if (gameState == GameState::Playing && explorer && mummy) {
        if (explorer->getX() == mummy->getX() && explorer->getY() == mummy->getY()) {
            gameState = GameState::Lost;
            // Tạo lost panel
            if (!lostPanel) {
                lostPanel = new LostPanel(renderer);
                if (lostPanel->init(winW, winH, [this]() {
                    // Play again callback - restart current level
                    cleanupForRestart();
                    init(currentStage);
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
    if (g_audioInstance) {
        g_audioInstance->cleanup();
        delete g_audioInstance;
        g_audioInstance = nullptr;
    }
    
    delete map;
    map = nullptr;

    delete explorer;
    explorer = nullptr;

    delete mummy;
    mummy = nullptr;

    SDL_DestroyRenderer(renderer);
    renderer = nullptr;

    SDL_DestroyWindow(window);
    window = nullptr;

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
    
    // Reset game state
    gameState = GameState::Playing;  // Thêm dòng này
    turn = 0;  // Thêm dòng này
    mummyStepsLeft = 0;  // Thêm dòng này
    settingsVisible = false;  // Thêm dòng này
    
    // KHÔNG destroy window và renderer - giữ lại để restart
    // KHÔNG gọi SDL_Quit(), TTF_Quit(), và không xóa g_audioInstance

    undoStack.clear();
    redoStack.clear();
}
void Game::run(char stage)
{
    init(stage);
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
            start.run();
        })) {
        delete settingsPanel;
        settingsPanel = nullptr;
        return;
    }
    int px = (winW - panelW) / 2;
    int py = (winH - panelH) / 2;
    settingsPanel->setPosition(px, py);
    settingsVisible = true;
}

void Game::restoreState(const State& state) {
    // Đặt lại vị trí nhân vật
    explorer->moveTo(state.expX, state.expY);
    mummy->moveTo(state.mumX, state.mumY);

    // Reset các biến điều khiển lượt để game không bị loạn
    turn = 0;             // Trả về lượt người chơi
    mummyStepsLeft = 0;   // Mummy dừng lại
    explorer->resetMoveFlag(); // Xóa cờ đã di chuyển
    
    // Nếu đang ở màn hình Thua/Thắng mà Undo thì quay lại trạng thái Playing
    if (gameState != GameState::Playing) {
        gameState = GameState::Playing;
        // Xóa các panel thông báo nếu có
        if (victoryPanel) { delete victoryPanel; victoryPanel = nullptr; }
        if (lostPanel) { delete lostPanel; lostPanel = nullptr; }
    }
}

void Game::performUndo() {
    if (undoStack.empty()) return;

    // 1. Lưu trạng thái hiện tại vào Redo Stack trước khi lùi lại
    State currentState = { explorer->getX(), explorer->getY(), mummy->getX(), mummy->getY() };
    redoStack.push_back(currentState);

    // 2. Lấy trạng thái cũ từ Undo Stack
    State prevState = undoStack.back();
    undoStack.pop_back();

    // 3. Khôi phục
    restoreState(prevState);
    std::cout << "Undo performed. Stack size: " << undoStack.size() << "\n";
}

void Game::performRedo() {
    if (redoStack.empty()) return;

    // 1. Lưu trạng thái hiện tại vào Undo Stack trước khi tiến tới
    State currentState = { explorer->getX(), explorer->getY(), mummy->getX(), mummy->getY() };
    undoStack.push_back(currentState);

    // 2. Lấy trạng thái tương lai từ Redo Stack
    State nextState = redoStack.back();
    redoStack.pop_back();

    // 3. Khôi phục
    restoreState(nextState);
    std::cout << "Redo performed.\n";
}