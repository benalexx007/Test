#pragma once
#include "../game.h"
#include <string>

class Game;

void undo(Game* game);
void redo(Game* game);
void reset(Game* game);
void settings(Game* game);
