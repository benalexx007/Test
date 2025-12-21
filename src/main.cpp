// Standard Simple DirectMedia Layer (SDL) and related libraries
// are used here for cross-platform multimedia and windowing.
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

// Project-specific headers that provide the core game object
// and the initial screen (Start) implementation.
#include "game.h"
#include "screens/start.h"

// Audio subsystem wrapper for background music and sound effects.
#include "core/audio.h"
#include "core/app_events.h"

// Global pointer to an Audio instance. The definition with an
// initializer ensures a single global instance pointer exists
// across translation units. It is used to allow persistence of
// the audio subsystem throughout the application's lifetime.
Audio* g_audioInstance = nullptr;

// Entry point of the program. The function initializes SDL
// subsystems required by the application, constructs the initial
// screen, runs the main UI loop, and performs orderly shutdown.
Uint32 gAppReturnToStartEvent = 0;
bool gReturnToStartRequested = false;

int main(int argc, char** argv) {
    // Pointer to the main application window. Initialized to nullptr
    // to make ownership and lifetime explicit.
    SDL_Window* window = nullptr;

    // Initialize the SDL video subsystem. This prepares SDL to
    // create windows, render contexts, and receive window events.
    SDL_Init(SDL_INIT_VIDEO);

    // Initialize TrueType font support. This function call is part
    // of SDL_ttf and is required before using any font rendering.
    TTF_Init();

    // Initialize the audio subsystem and create the global audio
    // instance if it does not already exist. Persisting a single
    // Audio object for the program's duration allows continuous
    // background music playback across different screens.
    if (!g_audioInstance) {
        // Initialize the SDL audio subsystem prior to constructing
        // the Audio wrapper which depends on it.
        SDL_Init(SDL_INIT_AUDIO);

        // Allocate the Audio wrapper object. This object encapsulates
        // audio device initialization, resource loading, and playback
        // semantics for the application.
        g_audioInstance = new Audio();

        // Attempt to initialize the audio wrapper. The init() method
        // typically opens audio devices and prepares the subsystem for
        // loading and playing sounds.
        if (g_audioInstance->init()) {
            // Attempt to load a background music asset. If loading
            // succeeds, start playback in a loop so that music continues
            // for the duration of the gameplay unless explicitly stopped.
            if (g_audioInstance->loadBackgroundMusic("assets/audio/background_music.wav")) {
                g_audioInstance->playBackgroundMusic(true); // Loop playback
            } else {
                // Log an error to standard error output. In a production
                // system a more robust logging facility would be preferred.
                std::cerr << "Failed to load background music\n";
            }
        }
    }

    // Allocate one application-defined event type for returning to Start
    // from in-game panels.
    gAppReturnToStartEvent = SDL_RegisterEvents(1);
    if (gAppReturnToStartEvent == 0) {
        std::cerr << "main: failed to register app event\n";
    } else {
        std::cerr << "main: registered app event=" << gAppReturnToStartEvent << "\n";
    }

    // Create the main application window with a title and an initial
    // size. The SDL_WINDOW_RESIZABLE flag allows the user to resize
    // the window; the program then maximizes it to occupy the screen.
    window = SDL_CreateWindow("Mê Cung Tây Du", 1920, 911, SDL_WINDOW_RESIZABLE);
    SDL_MaximizeWindow(window);

    // Construct the `Start` screen and run its event/update/render
    // loop. The `run` method encapsulates the life-cycle of the
    // initial screen (e.g., menu interactions) and returns when the
    // user leaves that screen or starts the game proper.
    Start start;
    std::cerr << "main: entering Start.run\n";
    start.run(window);
    std::cerr << "main: Start.run returned\n";

    // After the start screen completes, destroy the SDL window and
    // release associated resources. Explicitly null the pointer to
    // avoid accidental use after deletion.
    SDL_DestroyWindow(window);
    window = nullptr;

    // Clean up the global audio instance if it was created. This
    // includes freeing loaded audio assets and closing audio devices.
    if (g_audioInstance) {
        g_audioInstance->cleanup();
        delete g_audioInstance;
        g_audioInstance = nullptr;
    }

    // Shut down SDL subsystems in the reverse order of initialization.
    // This helps ensure that subsystem resources are released cleanly.
    SDL_Quit();
    TTF_Quit();

    // Return zero to indicate successful program termination.
    return 0;
}