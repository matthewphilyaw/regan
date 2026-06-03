//
// Created by Matthew Philyaw on 5/30/26.
//

#include "engine.hpp"
#include "raylib.h"
#include "menu/menu.hpp"
#include "editor/editor.hpp"

void regan::Engine::run() {
    while (!WindowShouldClose()) {
        if (new_engine_state_ != engine_state_) {
            transition_to_state(new_engine_state_);
            continue;
        }

        if (engine_state_ == EngineState::None) {
            continue;
        }

        switch (engine_state_) {
            case EngineState::MainMenu:
                menu_->update();
                break;
            case EngineState::Editor:
                editor_->update();
                break;
            case EngineState::Exit:
                return;
            default:
                break;
        }

        BeginDrawing();
        switch (engine_state_) {
            case EngineState::MainMenu:
                menu_->draw();
                break;
            case EngineState::Editor:
                editor_->draw();
                break;
            case EngineState::Exit:
                return;
            default:
                break;
        }
        EndDrawing();
    }
}

void regan::Engine::request_transition_to_state(EngineState new_state) {
    new_engine_state_ = new_state;
}

void regan::Engine::transition_to_state(EngineState new_state) {
    engine_state_ = new_state;

    editor_.reset();
    menu_.reset();

    switch (engine_state_) {
        case EngineState::MainMenu:
            menu_ = std::make_unique<Menu>(*this);
            break;
        case EngineState::Editor:
            editor_ = std::make_unique<editor::Editor>(*this);
            break;
        default:
            break;
    }
}
