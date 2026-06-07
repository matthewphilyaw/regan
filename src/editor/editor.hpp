#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "command.hpp"
#include "entity.hpp"
#include "rlImGui.h"
#include "imgui.h"
#include "ImGuizmo.h"
#include "light_system.hpp"
#include "registry.hpp"
#include "common/managed_model.hpp"
#include "common/managed_texture_2d.hpp"

namespace regan {

    class Engine;
}

namespace regan::editor {
    constexpr const char* ASSET_PATH = "/Users/mphilyaw/code/regan/assets";

    struct UniformLocations {
        int lightCount;
        int ambient;
        int edgeFade;
        int matModel;
        int matNormal;
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

        void draw();
        ~Editor();

    private:
        void lock_cursor();
        void unlock_cursor();

        void gui_draw_menu_bar();
        void gui_draw_outliner_panel();
        void gui_draw_entity_properties();
        void gui_draw_entity_transform_properties(Entity &entity);
        void gui_draw_entity_light_properties(Entity &entity);
        void gui_draw_perf_overlay();

        void add_light_entity();
        void add_model_entity(std::string model_path, std::string texture_path);

        void remove_entity(size_t id);

        void update_shortcuts();

        void gui_draw_add_model_popup();

        void duplicate_entity(size_t id);

        void populate_file_picker(const std::filesystem::path &dir, const std::vector<std::string> &extensions);

        void gui_draw_file_picker();

        void update_camera();
        void update_selection();

        void draw_gizmo();
        void draw_grid();
        void draw_entities();
        void draw_selection_highlight();
        void draw_light_gizmos();

        bool show_add_model_popup_ = false;
        std::string pending_model_path_;
        std::string pending_texture_path_;

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
        UniformLocations uniform_locs_{};

        int mat_model_loc_  = -1;
        int mat_normal_loc_ = -1;
        int ambient_loc_    = -1;
        int edge_fade_loc_ = -1;

        Shader lighting_shader_{};
        LightSystem light_system_;
        CommandHistory history_{};
    };
}
