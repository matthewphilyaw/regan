//
// Created by Matthew Philyaw on 5/30/26.
//

#include "engine.hpp"
#include "raylib.h"
#include "menu/menu.hpp"
#include "editor/editor.hpp"

void regan::Engine::run() {
    while (!WindowShouldClose()) {
        if (this->new_engine_state != this->engine_state) {
            this->transition_to_state(this->new_engine_state);
            continue;
        }

        if (this->engine_state == EngineState::None) {
            continue;
        }

        switch (engine_state) {
            case MainMenuState:
                this->menu->update();
                break;
            case EditorState:
                this->editor->update();
                break;
            case ExitState:
                return;
            default:
                break;
        }

        BeginDrawing();
        switch (engine_state) {
            case MainMenuState:
                this->menu->draw();
                break;
            case EditorState:
                this->editor->draw();
                break;
            case ExitState:
                return;
            default:
                break;
        }
        EndDrawing();
    }
}

void regan::Engine::request_transition_to_state(EngineState new_state) {
    this->new_engine_state = new_state;
}

void regan::Engine::transition_to_state(EngineState new_state) {
    this->engine_state = new_state;

    this->editor.reset();
    this->menu.reset();

    switch (engine_state) {
        case MainMenuState:
            this->menu = std::make_unique<Menu>(*this);
            break;
        case EditorState:
            this->editor = std::make_unique<Editor>(*this);
            break;
        default:
            break;
    }
}
