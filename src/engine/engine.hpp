#pragma once

#include <memory>
#include "editor/editor.hpp"
#include "menu/menu.hpp"

namespace regan {
    class Menu;
    class Editor;

    enum class EngineState {
        None,
        MainMenu,
        Editor,
        Exit
    };

    class Engine {
        public:
            void run();
            void request_transition_to_state(EngineState engine_state);

        private:
            void transition_to_state(EngineState engine_state);

            EngineState engine_state_ = EngineState::None;
            EngineState new_engine_state_ = EngineState::None;

            std::unique_ptr<Menu> menu_;
            std::unique_ptr<editor::Editor> editor_;
    };
}