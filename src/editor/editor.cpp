#include "editor.hpp"

#include <cmath>
#include <sys/_select.h>

#include "imgui.h"
#include "raylib.h"
#include "raymath.h"
#include "rlImGui.h"
#include "ImGuizmo.h"
#include "engine/engine.hpp"
#include <filesystem>

#include "rlgl.h"

namespace fs = std::filesystem;

fs::path asset_path = "/Users/mphilyaw/code/regan/assets";

namespace regan {

    static Matrix matrix_from_floats(const float f[16]) {
        return Matrix{
            f[0],  f[1],  f[2],  f[3],
            f[4],  f[5],  f[6],  f[7],
            f[8],  f[9],  f[10], f[11],
            f[12], f[13], f[14], f[15]
        };
    }

    void EditorCamera::update_camera_position(Camera3D &camera) const {
        camera.position = {
            focal_point.x + distance * cosf(pitch) * sinf(yaw),
            focal_point.y + distance * sinf(pitch),
            focal_point.z + distance * cosf(pitch) * cosf(yaw)
        };
        camera.target = focal_point;
        camera.up = {0.0f, 1.0f, 0.0f};
        camera.fovy = fov;
        camera.projection = CAMERA_PERSPECTIVE;
    }

    Editor::Editor(Engine &engine) : engine(engine) {
        EnableCursor();
        SetMouseCursor(MOUSE_CURSOR_CROSSHAIR);
        rlImGuiSetup(true);
        editor_camera.update_camera_position(camera_3d);

        // join paths with /
        size_t model_id = this->models.size();
        size_t texture_id = this->textures.size();

        this->models.push_back(LoadModel((asset_path / "kit" / "sm_ok_x3_wm_s.glb").string().c_str()));
        this->textures.push_back(LoadTexture((asset_path / "textures" / "office_kit.png").string().c_str()));

        SetTextureFilter(this->textures[texture_id], RL_TEXTURE_FILTER_POINT);
        this->models[model_id].materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = this->textures[texture_id];

        size_t obj_id = this->obj_nodes.size();
        this->obj_nodes.push_back({
            .model_id = model_id,
            .texture_id = texture_id,
            .position = {0, 0, 0},
            .rotation = {0, 0, 0},
            .scale = {1, 1, 1},
            .tint = WHITE
        });


        this->scene_node_list.push_back({
            .name = "Test",
            .type = Obj,
            .id = obj_id
        });

        obj_id = this->obj_nodes.size();
        this->obj_nodes.push_back({
            .model_id = model_id,
            .texture_id = texture_id,
            .position = {5, 0, 0},
            .rotation = {0, 0, 0},
            .scale = {1, 1, 1},
            .tint = WHITE
        });

        this->scene_node_list.push_back({
            .name = "Test 2",
            .type = Obj,
            .id = obj_id
        });
    }

    Editor::~Editor() {
        rlImGuiShutdown();
    }

    void Editor::lock_cursor() {
        if (!cursor_locked) {
            saved_cursor_pos = GetMousePosition();
            DisableCursor();
            cursor_locked = true;
        }
    }

    void Editor::unlock_cursor() {
        if (cursor_locked) {
            EnableCursor();
            SetMousePosition(
                static_cast<int>(saved_cursor_pos.x),
                static_cast<int>(saved_cursor_pos.y)
            );
            SetMouseCursor(MOUSE_CURSOR_CROSSHAIR);
            cursor_locked = false;
        }
    }

    void Editor::update() {
        update_camera();

        if (IsKeyPressed(KEY_G)) {
            this->operation = ImGuizmo::TRANSLATE;
        }
        if (IsKeyPressed(KEY_R)) {
            this->operation = ImGuizmo::ROTATE;
        }
        if (IsKeyPressed(KEY_S)) {
            this->operation = ImGuizmo::SCALE;
        }
    }

    void Editor::draw() {
        ClearBackground({20, 20, 20, 255});

        BeginMode3D(camera_3d);

        for (int x = -10; x <= 10; x++) {
            DrawLine3D(
                {x * 2.56f, 0.0f, -10 * 2.56f},
                {x * 2.56f, 0.0f, 10 * 2.56f},
                {80, 80, 80, 255}
            );
        }
        for (int z = -10; z <= 10; z++) {
            DrawLine3D(
                {-10 * 2.56f, 0.0f, z * 2.56f},
                {10 * 2.56f, 0.0f, z * 2.56f},
                {80, 80, 80, 255}
            );
        }

        DrawLine3D({0, 0, 0}, {1, 0, 0}, RED);
        DrawLine3D({0, 0, 0}, {0, 1, 0}, GREEN);
        DrawLine3D({0, 0, 0}, {0, 0, 1}, BLUE);

        for (auto& node : this->obj_nodes) {
            Model& model = this->models[node.model_id];

            Matrix transform = MatrixMultiply(
                MatrixMultiply(
                    MatrixScale(node.scale.x, node.scale.y, node.scale.z),
                    QuaternionToMatrix(node.rotation)
                ),
                MatrixTranslate(node.position.x, node.position.y, node.position.z)
            );

            model.transform = transform;
            DrawModel(model, {0, 0, 0}, 1.0, node.tint);
        }

        EndMode3D();

        rlImGuiBegin();
        ImGuizmo::SetOrthographic(false);
        ImGuizmo::BeginFrame();
        ImGuizmo::SetRect(0, 0, GetScreenWidth(), GetScreenHeight());

        this->draw_menu_bar();
        this->draw_outliner_panel();
        this->draw_node_properties();
        if (selected.has_value()) {
            auto& node = this->scene_node_list[selected.value()];
            auto& obj = this->obj_nodes[node.id];

            // build matrix from current object state
            Matrix transform = MatrixMultiply(
                MatrixMultiply(
                    MatrixScale(obj.scale.x, obj.scale.y, obj.scale.z),
                    QuaternionToMatrix(obj.rotation)
                ),
                MatrixTranslate(obj.position.x, obj.position.y, obj.position.z)
            );

            Matrix view =  GetCameraMatrix(this->camera_3d);
            Matrix projection = MatrixPerspective(
                this->camera_3d.fovy * DEG2RAD,
                (float)GetScreenWidth() / GetScreenHeight(),
                0.01f,
                1000.0f
            );

            float view_f[16];
            float proj_f[16];
            float transform_f[16];

            memcpy(view_f, MatrixToFloat(view), 16 * sizeof(float));
            memcpy(proj_f, MatrixToFloat(projection), 16 * sizeof(float));
            memcpy(transform_f, MatrixToFloat(transform), 16 * sizeof(float));

            // store previous transform
            float prev_transform_f[16];
            memcpy(prev_transform_f, transform_f, 16 * sizeof(float));

            if (ImGuizmo::Manipulate(view_f, proj_f, this->operation, ImGuizmo::WORLD, transform_f)) {
                // extract just the delta rotation
                Matrix prev_mat = matrix_from_floats(prev_transform_f);
                Matrix curr_mat = matrix_from_floats(transform_f);

                // delta = inverse(prev) * curr
                Matrix delta = MatrixMultiply(curr_mat, MatrixInvert(prev_mat));

                // apply delta to current quaternion
                Quaternion delta_q = QuaternionFromMatrix(delta);
                delta_q = QuaternionInvert(delta_q);
                obj.rotation = QuaternionNormalize(QuaternionMultiply(delta_q,obj.rotation));

                // update euler for display
                Vector3 e = QuaternionToEuler(obj.rotation);
                obj.euler_degrees = {e.x * RAD2DEG, e.y * RAD2DEG, e.z * RAD2DEG};

                // position and scale still from decompose
                float t[3], r[3], s[3];
                ImGuizmo::DecomposeMatrixToComponents(transform_f, t, r, s);
                obj.position = {t[0], t[1], t[2]};
                obj.scale    = {s[0], s[1], s[2]};
            }
        }
        rlImGuiEnd();
    }

    void Editor::update_camera() {
        if (ImGui::GetIO().WantCaptureMouse) return;

        constexpr float sensitivity = 0.005f;
        constexpr float pan_speed = 0.005f;
        constexpr float zoom_speed = 0.5f;

        bool middle_down = IsMouseButtonDown(MOUSE_MIDDLE_BUTTON);
        bool middle_pressed = IsMouseButtonPressed(MOUSE_MIDDLE_BUTTON);
        bool middle_released = IsMouseButtonReleased(MOUSE_MIDDLE_BUTTON);
        bool shift_down = IsKeyDown(KEY_LEFT_SHIFT);

        // Middle mouse — orbit or pan
        if (middle_down) {
            if (middle_pressed) {
                // Lock cursor, skip delta this frame to avoid jump
                lock_cursor();
            } else if (!shift_down) {
                // Orbit
                Vector2 delta = GetMouseDelta();
                editor_camera.yaw -= delta.x * sensitivity;
                editor_camera.pitch += delta.y * sensitivity;
                editor_camera.pitch = Clamp(editor_camera.pitch, -1.5f, 1.5f);
                editor_camera.update_camera_position(camera_3d);
            } else {
                // Shift + middle — pan
                Vector2 delta = GetMouseDelta();

                // Current — uses world up, wrong at angles
                Vector3 up = camera_3d.up;  // always {0, 1, 0}

                // Fix — compute camera local up from forward and right
                Vector3 forward = Vector3Normalize(
                    Vector3Subtract(camera_3d.target, camera_3d.position)
                );
                Vector3 right = Vector3Normalize(
                    Vector3CrossProduct(forward, { 0.0f, 1.0f, 0.0f })
                );
                Vector3 cam_up = Vector3Normalize(
                    Vector3CrossProduct(right, forward)
                );

                editor_camera.focal_point = Vector3Add(
                    editor_camera.focal_point,
                    Vector3Scale(right, -delta.x * pan_speed * editor_camera.distance)
                );
                editor_camera.focal_point = Vector3Add(
                    editor_camera.focal_point,
                    Vector3Scale(cam_up, delta.y * pan_speed * editor_camera.distance)
                );
                editor_camera.update_camera_position(camera_3d);
            }
        }

        if (middle_released) {
            unlock_cursor();
        }

        // Scroll — dolly
        float scroll = GetMouseWheelMove();
        if (scroll != 0.0f) {
            editor_camera.distance -= scroll * zoom_speed;
            editor_camera.distance = Clamp(editor_camera.distance, 0.5f, 100.0f);
            editor_camera.update_camera_position(camera_3d);
        }
    }

    void Editor::draw_menu_bar() const {
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("Exit to Menu")) {
                    engine.request_transition_to_state(MainMenuState);
                }
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }
    }

    void Editor::draw_outliner_panel() {
        ImGui::SetNextWindowSize(ImVec2(300, 600), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos(ImVec2(0, 20), ImGuiCond_FirstUseEver);
        ImGui::Begin("Outline");

        if (ImGui::BeginListBox("##outliner", ImVec2(-1, -1))) {

            for (size_t index = 0; index < this->scene_node_list.size(); index++) {
                ImGui::PushID(static_cast<int>(index));
                bool current_selection = this->selected.has_value() && this->selected.value() == index;
                if (ImGui::Selectable(this->scene_node_list[index].name.c_str(), current_selection)) {
                    this->selected = index;
                }
                ImGui::PopID();
            }
            ImGui::EndListBox();
        }

        ImGui::End();
    }

    void Editor::draw_node_properties() {
        ImVec2 screen = ImGui::GetIO().DisplaySize;
        float panel_width = 300.0f;

        ImGui::SetNextWindowSize(ImVec2(panel_width, 600), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos(ImVec2(screen.x - panel_width, 20), ImGuiCond_FirstUseEver);
        ImGui::Begin("Node Properties");

        if (!this->selected.has_value() || this->scene_node_list.empty()) {
            ImGui::End();
            return;
        }

        switch (this->scene_node_list[this->selected.value()].type) {
            case Obj:
                this->draw_obj_node();
                break;
        }

        ImGui::End();
    }

    void Editor::draw_obj_node() {
        if (!this->selected.has_value()) {
            return;
        }

        if (!this->selected.has_value() || this->scene_node_list[selected.value()].type != Obj) {
            return;
        }

        if (this->obj_nodes.size() < this->selected.value()) {
            return;
        }

        ObjNode& node = this->obj_nodes[this->selected.value()];

        ImGui::BeginGroup();
        ImGui::DragFloat3("Position", reinterpret_cast<float*>(&node.position), -100, 100);

        ImGui::DragFloat3("Rotation", &node.euler_degrees.x, -100, 100);
        node.rotation = QuaternionFromEuler(
            node.euler_degrees.x * DEG2RAD,
            node.euler_degrees.y * DEG2RAD,
            node.euler_degrees.z * DEG2RAD
        );

        ImGui::DragFloat3("Scale", reinterpret_cast<float*>(&node.scale), -100, 100);
        ImGui::EndGroup();
    }
} // namespace regan
