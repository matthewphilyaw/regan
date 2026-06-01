#pragma once

#include <memory>
#include "editor/editor.hpp"
#include "menu/menu.hpp"

namespace regan {
    class Menu;
    class Editor;

    enum EngineState {
        None,
        MainMenuState,
        EditorState,
        ExitState
    };

    class Engine {
        public:
            void run();
            void request_transition_to_state(EngineState engine_state);

        private:
            void transition_to_state(EngineState engine_state);

            EngineState engine_state = None;
            EngineState new_engine_state = None;

            std::unique_ptr<Menu> menu;
            std::unique_ptr<Editor> editor;
    };
}