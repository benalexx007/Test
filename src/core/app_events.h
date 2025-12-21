#pragma once
#include <SDL3/SDL.h>

// Custom application event for returning to Start screen.
extern Uint32 gAppReturnToStartEvent;
// Global flag set when in-game requests to return to Start (reliable fallback)
extern bool gReturnToStartRequested;
// Simple fallback flag to request returning to Start when SDL events
// may be lost during cleanup.
extern bool gReturnToStartRequested;
