#include "functions.h"

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