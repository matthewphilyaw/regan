#pragma once

#include <string>
#include <vector>

#include "entity.hpp"
#include "rlImGui.h"
#include "imgui.h"
#include "ImGuizmo.h"
#include "registry.hpp"
#include "common/managed_model.hpp"
#include "common/managed_texture_2d.hpp"

namespace regan {

    class Engine;
}

namespace regan::editor {
    constexpr const char* ASSET_PATH = "/Users/mphilyaw/code/regan/assets";

    struct EditorCamera {
        Vector3 focal_point = {0.0f, 0.0f, 0.0f};
        float distance = 10.0f;
        float yaw = 0.0f;
        float pitch = 0.3f;
        float fov = 70.0f;

        void update_camera_position(Camera3D &camera) const;
    };

    class Editor {
    public:
        explicit Editor(regan::Engine &engine);
        void update();

        void draw_selection_highlight();

        void draw();
        ~Editor();

    private:
        void draw_menu_bar() const;
        void draw_outliner_panel();
        void gui_draw_entity_properties();
        void gui_draw_entity_transform_properties(Entity &entity);
        void update_camera();
        void lock_cursor();
        void unlock_cursor();

        void update_selection();

        void draw_gizmo();
        void draw_grid();
        void draw_entities();

        Engine &engine_;
        Camera3D camera_3d_{};
        EditorCamera editor_camera_{};
        Vector2 saved_cursor_pos_{};

        Registry<Entity> entities_{};
        Registry<common::ManagedModel> models_{};
        Registry<common::ManagedTexture2d> textures_{};

        std::optional<size_t> selected_{};
        bool cursor_locked_ = false;
        ImGuizmo::OPERATION operation_ = ImGuizmo::TRANSLATE;
    };
}
