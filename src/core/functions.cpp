#include "functions.h"

// Each of the following functions is a thin and well-documented
// forwarding adapter that invokes the corresponding member method
// on the `Game` instance. They are implemented as free functions
// so they can be easily bound to UI callbacks that expect plain
// function pointers or `std::function` targets.
void undo(Game* game) {
    if (game) {
        game->undo();
    }
}

void redo(Game* game) {
    if (game) {
        game->redo();
    }
}

void reset(Game* game) {
    if (game) {
        game->reset();
    }
}

void settings(Game* game) {
    if (game) {
        game->toggleSettings();
    }
}