// Small set of free functions that serve as adapters between UI
// callbacks (which often require plain function pointers) and the
// `Game` class member methods. Each function simply checks the
// provided `Game*` pointer for null and forwards the call.
#pragma once
#include "../game.h"
#include <string>

class Game;

// Forwarding functions used as button callbacks in UI panels.
void undo(Game* game);
void redo(Game* game);
void reset(Game* game);
void settings(Game* game);
