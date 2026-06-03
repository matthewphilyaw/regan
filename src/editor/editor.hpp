#pragma once

#include <string>
#include <vector>

#include "rlImGui.h"
#include "imgui.h"
#include "ImGuizmo.h"
#include "registry.hpp"

namespace regan {
    class Engine;
}

namespace regan::editor {
    constexpr const char* ASSET_PATH = "/Users/mphilyaw/code/regan/assets";

    enum NodeType {
        Obj
    };

    struct ObjNode {
        size_t model_id;
        size_t texture_id;
        Vector3 position;
        Vector3 euler_degrees;
        Quaternion rotation;
        Vector3 scale;
        Color tint;
    };

    struct SceneNode {
        std::string name;
        NodeType type;
        size_t id;
    };

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
        void draw_node_properties();
        void draw_obj_node();
        void update_camera();
        void lock_cursor();
        void unlock_cursor();

        void update_selection();

        void draw_gizmo();
        void draw_grid();
        void draw_nodes();

        Engine &engine_;
        Camera3D camera_3d_{};
        EditorCamera editor_camera_{};
        Vector2 saved_cursor_pos_{};

        std::vector<SceneNode> scene_node_list_{};
        Registry<ObjNode> obj_nodes_{};
        Registry<Model> models_{};
        Registry<Texture2D> textures_{};

        std::optional<size_t> selected_{};
        bool cursor_locked_ = false;
        ImGuizmo::OPERATION operation_ = ImGuizmo::TRANSLATE;
    };
}
