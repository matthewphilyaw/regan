//
// Created by Matthew Philyaw on 5/30/26.
//

#include "menu.hpp"

#include "engine/engine.hpp"

namespace regan {
    Menu::Menu(Engine &engine) : engine(engine) {
        EnableCursor();

        float cx = GetScreenWidth() * 0.5f;
        float cy = GetScreenHeight() * 0.5f;

        this->play_btn = {cx - 100, cy - 30, 200, 40};
        this->editor_btn = {cx - 100, cy + 30, 200, 40};
        this->quit_btn = {cx - 100, cy + 90, 200, 40};

        SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
    }

    bool Menu::button(const char *label, Rectangle rect) {
        Vector2 mouse = GetMousePosition();
        bool hover = CheckCollisionPointRec(mouse, rect);
        bool click = hover && IsMouseButtonPressed(MOUSE_LEFT_BUTTON);

        // Background
        Color bg = hover
                       ? Color{60, 60, 60, 255}
                       : Color{30, 30, 30, 255};

        DrawRectangleRec(rect, bg);
        DrawRectangleLinesEx(rect, 1, {80, 80, 80, 255});

        // Center text in button
        int fontSize = 16;
        int textWidth = MeasureText(label, fontSize);
        int textX = static_cast<int>(rect.x + (rect.width - textWidth) / 2);
        int textY = static_cast<int>(rect.y + (rect.height - fontSize) / 2);

        DrawText(label, textX, textY, fontSize, hover ? WHITE : GRAY);

        return click;
    }

    void Menu::update() {
        // Hit test only — no drawing
        Vector2 mouse = GetMousePosition();
        bool click = IsMouseButtonPressed(MOUSE_LEFT_BUTTON);

        if (click) {
            if (CheckCollisionPointRec(mouse, this->play_btn)) {
                this->engine.request_transition_to_state(EngineState::MainMenuState);
            }
            if (CheckCollisionPointRec(mouse, this->editor_btn)) {
                this->engine.request_transition_to_state(EngineState::EditorState);
            }
            if (CheckCollisionPointRec(mouse, this->quit_btn)) {
                this->engine.request_transition_to_state(EngineState::ExitState);
            }
        }

    }

    void Menu::draw() {
        ClearBackground({20, 20, 20, 255});

        float cx = GetScreenWidth() * 0.5f;
        float cy = GetScreenHeight() * 0.5f;

        // Title
        const char *title = "REGAN";
        int titleSize = 32;
        int titleW = MeasureText(title, titleSize);
        DrawText(title,
                 static_cast<int>(cx - titleW * 0.5f),
                 static_cast<int>(cy - 120),
                 titleSize, WHITE);

        button("PLAY", this->play_btn);
        button("EDITOR", this->editor_btn);
        button("QUIT", this->quit_btn);
    }
} // namespace regan