#pragma once

#include <string>
#include <vector>

#include "rlImGui.h"
#include "imgui.h"
#include "ImGuizmo.h"

namespace regan {
    class Engine;
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
        explicit Editor(Engine &engine);
        void update();
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

        Engine &engine;
        Camera3D camera_3d{};
        EditorCamera editor_camera{};
        Vector2 saved_cursor_pos{};

        std::vector<SceneNode> scene_node_list{};
        std::vector<ObjNode> obj_nodes{};
        std::vector<Model> models{};
        std::vector<Texture2D> textures{};

        std::optional<size_t> selected{};
        bool cursor_locked = false;
        ImGuizmo::OPERATION operation = ImGuizmo::TRANSLATE;
    };
}
