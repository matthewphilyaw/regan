#include "raylib.h"
#include "engine/engine.hpp"


int main() {
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(1280, 720, "Regan");
    //SetTargetFPS(60);
    SetExitKey(KEY_NULL);

    {
        regan::Engine engine;
        engine.request_transition_to_state(regan::EngineState::MainMenu);
        engine.run();
    }

    return 0;
}

