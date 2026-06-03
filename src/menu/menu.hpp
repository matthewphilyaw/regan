//
// Created by Matthew Philyaw on 5/30/26.
//

#pragma once


#pragma once
#include "raylib.h"

namespace regan {

    class Engine;

    class Menu {
    public:
        explicit Menu(Engine &engine);
        void update();
        void draw();

    private:
        Engine &engine_;
        Rectangle play_btn_;
        Rectangle editor_btn_;
        Rectangle quit_btn_;
        // Button helper
        bool button(const char* label, Rectangle rect);
    };

} // namespace regan