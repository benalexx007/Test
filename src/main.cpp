#include "game.h"
#include "start.h"
#include "audio.h"

// Khai báo global audio instance để chia sẻ giữa Start và Game
extern Audio* g_audioInstance;
int main(int argc, char** argv) {
    Game game;
    std::string stage = "1";
    game.run(stage);
    // Start start;
    // start.run();
    // return 0;
}