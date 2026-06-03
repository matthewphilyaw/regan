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
#include <ranges>

#include "rlgl.h"

namespace fs = std::filesystem;

fs::path asset_path = "/Users/mphilyaw/code/regan/assets";

namespace regan::editor {

    static Matrix matrix_from_floats(const float f[16]) {
        return Matrix{
            f[0],  f[1],  f[2],  f[3],
            f[4],  f[5],  f[6],  f[7],
            f[8],  f[9],  f[10], f[11],
            f[12], f[13], f[14], f[15]
        };
    }

    static Matrix node_transform(const ObjNode& n) {
        return MatrixMultiply(
            MatrixMultiply(
                MatrixScale(n.scale.x, n.scale.y, n.scale.z),
                QuaternionToMatrix(n.rotation)
            ),
            MatrixTranslate(n.position.x, n.position.y, n.position.z)
        );
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

    Editor::Editor(Engine &engine) : engine_(engine) {
        EnableCursor();
        SetMouseCursor(MOUSE_CURSOR_CROSSHAIR);
        rlImGuiSetup(true);
        editor_camera_.update_camera_position(camera_3d_);

        auto model_id = models_.add(LoadModel((asset_path / "kit" / "sm_ok_x3_wm_s.glb").string().c_str()));
        auto texture_id = textures_.add(LoadTexture((asset_path / "textures" / "office_kit.png").string().c_str()));

        SetTextureFilter(*textures_.get(texture_id), RL_TEXTURE_FILTER_POINT);
        models_.get(model_id)->materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = *textures_.get(texture_id);

        auto obj_id = obj_nodes_.add({
            .model_id = model_id,
            .texture_id = texture_id,
            .position = {0, 0, 0},
            .euler_degrees = {0, 0, 0},
            .rotation = QuaternionIdentity(),
            .scale = {1, 1, 1},
            .tint = WHITE
        });


        scene_node_list_.push_back({
            .name = "Test",
            .type = Obj,
            .id = obj_id
        });

        obj_id = obj_nodes_.add({
            .model_id = model_id,
            .texture_id = texture_id,
            .position = {5, 0, 0},
            .euler_degrees = {0, 0, 0},
            .rotation = QuaternionIdentity(),
            .scale = {1, 1, 1},
            .tint = WHITE
        });

        scene_node_list_.push_back({
            .name = "Test 2",
            .type = Obj,
            .id = obj_id
        });
    }

    Editor::~Editor() {
        rlImGuiShutdown();
    }

    void Editor::lock_cursor() {
        if (!cursor_locked_) {
            saved_cursor_pos_ = GetMousePosition();
            DisableCursor();
            cursor_locked_ = true;
        }
    }

    void Editor::unlock_cursor() {
        if (cursor_locked_) {
            EnableCursor();
            SetMousePosition(
                static_cast<int>(saved_cursor_pos_.x),
                static_cast<int>(saved_cursor_pos_.y)
            );
            SetMouseCursor(MOUSE_CURSOR_CROSSHAIR);
            cursor_locked_ = false;
        }
    }

    void Editor::update_selection() {
        if (ImGui::GetIO().WantCaptureMouse) return;
        if (!IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) return;
        if (ImGuizmo::IsOver()) return;  // don't deselect when grabbing the gizmo

        Ray ray = GetScreenToWorldRay(GetMousePosition(), camera_3d_);

        std::optional<size_t> hit_index;
        float closest = FLT_MAX;

        for (size_t i = 0; i < scene_node_list_.size(); i++) {
            auto& scene_node = scene_node_list_[i];
            ObjNode* node = obj_nodes_.get(scene_node.id);
            if (!node) continue;

            Model* model = models_.get(node->model_id);
            if (!model) continue;

            auto transform = node_transform(*node);

            RayCollision hit = GetRayCollisionMesh(ray, model->meshes[0], transform);
            if (hit.hit && hit.distance < closest) {
                closest = hit.distance;
                hit_index = i;
            }
        }

        selected_ = hit_index;  // nullopt if nothing hit — clicking empty space deselects
    }

    void Editor::update() {
        update_camera();
        update_selection();

        if (IsKeyPressed(KEY_G)) {
            operation_ = ImGuizmo::TRANSLATE;
        }
        if (IsKeyPressed(KEY_R)) {
            operation_ = ImGuizmo::ROTATE;
        }
        if (IsKeyPressed(KEY_S)) {
            operation_ = ImGuizmo::SCALE;
        }

    }

    void Editor::draw_selection_highlight() {
        if (!selected_.has_value()) return;

        auto& scene_node = scene_node_list_[selected_.value()];
        auto* obj = obj_nodes_.get(scene_node.id);
        if (!obj) return;

        Model* model = models_.get(obj->model_id);
        if (!model) return;

        BoundingBox box = GetModelBoundingBox(*model);
        Matrix transform = node_transform(*obj);

        rlPushMatrix();
        rlMultMatrixf(MatrixToFloat(transform));
        DrawBoundingBox(box, MAGENTA);
        rlPopMatrix();
    }

    void Editor::draw_gizmo() {
        ImGuizmo::SetOrthographic(false);
        ImGuizmo::BeginFrame();
        ImGuizmo::SetRect(0, 0, GetScreenWidth(), GetScreenHeight());
        if (selected_.has_value()) {
            auto& node = scene_node_list_[selected_.value()];
            auto* obj = obj_nodes_.get(node.id);

            auto transform = node_transform(*obj);

            Matrix view =  GetCameraMatrix(camera_3d_);
            Matrix projection = MatrixPerspective(
                camera_3d_.fovy * DEG2RAD,
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

            if (ImGuizmo::Manipulate(view_f, proj_f, operation_, ImGuizmo::WORLD, transform_f)) {
                // extract just the delta rotation
                Matrix prev_mat = matrix_from_floats(prev_transform_f);
                Matrix curr_mat = matrix_from_floats(transform_f);

                // delta = inverse(prev) * curr
                Matrix delta = MatrixMultiply(curr_mat, MatrixInvert(prev_mat));

                // apply delta to current quaternion
                Quaternion delta_q = QuaternionFromMatrix(delta);
                delta_q = QuaternionInvert(delta_q);
                obj->rotation = QuaternionNormalize(QuaternionMultiply(delta_q,obj->rotation));

                // update euler for display
                Vector3 e = QuaternionToEuler(obj->rotation);
                obj->euler_degrees = {e.x * RAD2DEG, e.y * RAD2DEG, e.z * RAD2DEG};

                // position and scale still from decompose
                float t[3], r[3], s[3];
                ImGuizmo::DecomposeMatrixToComponents(transform_f, t, r, s);
                obj->position = {t[0], t[1], t[2]};
                obj->scale    = {s[0], s[1], s[2]};
            }
        }
    }

    void Editor::draw_grid() {
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
    }

    void Editor::draw_nodes() {
        for (const auto &node: obj_nodes_ | std::views::values) {
            auto* model = models_.get(node.model_id);
            auto transform = node_transform(node);

            model->transform = transform;
            DrawModel(*model, {0, 0, 0}, 1.0, node.tint);
        }
    }

    void Editor::draw() {
        ClearBackground({20, 20, 20, 255});

        BeginMode3D(camera_3d_);

        draw_grid();
        draw_nodes();
        draw_selection_highlight();

        EndMode3D();

        rlImGuiBegin();

        draw_menu_bar();
        draw_outliner_panel();
        draw_node_properties();
        draw_gizmo();

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
                editor_camera_.yaw -= delta.x * sensitivity;
                editor_camera_.pitch += delta.y * sensitivity;
                editor_camera_.pitch = Clamp(editor_camera_.pitch, -1.5f, 1.5f);
                editor_camera_.update_camera_position(camera_3d_);
            } else {
                // Shift + middle — pan
                Vector2 delta = GetMouseDelta();

                // Current — uses world up, wrong at angles
                Vector3 up = camera_3d_.up;  // always {0, 1, 0}

                // Fix — compute camera local up from forward and right
                Vector3 forward = Vector3Normalize(
                    Vector3Subtract(camera_3d_.target, camera_3d_.position)
                );
                Vector3 right = Vector3Normalize(
                    Vector3CrossProduct(forward, { 0.0f, 1.0f, 0.0f })
                );
                Vector3 cam_up = Vector3Normalize(
                    Vector3CrossProduct(right, forward)
                );

                editor_camera_.focal_point = Vector3Add(
                    editor_camera_.focal_point,
                    Vector3Scale(right, -delta.x * pan_speed * editor_camera_.distance)
                );
                editor_camera_.focal_point = Vector3Add(
                    editor_camera_.focal_point,
                    Vector3Scale(cam_up, delta.y * pan_speed * editor_camera_.distance)
                );
                editor_camera_.update_camera_position(camera_3d_);
            }
        }

        if (middle_released) {
            unlock_cursor();
        }

        // Scroll — dolly
        float scroll = GetMouseWheelMove();
        if (scroll != 0.0f) {
            editor_camera_.distance -= scroll * zoom_speed;
            editor_camera_.distance = Clamp(editor_camera_.distance, 0.5f, 100.0f);
            editor_camera_.update_camera_position(camera_3d_);
        }
    }

    void Editor::draw_menu_bar() const {
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("Exit to Menu")) {
                    engine_.request_transition_to_state(EngineState::MainMenu);
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

            for (size_t index = 0; index < scene_node_list_.size(); index++) {
                ImGui::PushID(static_cast<int>(index));
                bool current_selection = selected_.has_value() && selected_.value() == index;
                if (ImGui::Selectable(scene_node_list_[index].name.c_str(), current_selection)) {
                    selected_ = index;
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

        if (!selected_.has_value() || scene_node_list_.empty()) {
            ImGui::End();
            return;
        }

        switch (scene_node_list_[selected_.value()].type) {
            case Obj:
                draw_obj_node();
                break;
        }

        ImGui::End();
    }

    void Editor::draw_obj_node() {
        if (!selected_.has_value() || scene_node_list_[selected_.value()].type != Obj) {
            return;
        }

        auto scene_node = scene_node_list_[selected_.value()];

        ObjNode* node = obj_nodes_.get(scene_node.id);
        if (node == nullptr) {
            return;
        }

        ImGui::BeginGroup();
        ImGui::DragFloat3("Position", reinterpret_cast<float*>(&node->position), 0.1);
        if (ImGui::DragFloat3("Rotation", &node->euler_degrees.x, 0.1)) {
            node->rotation = QuaternionFromEuler(
                node->euler_degrees.x * DEG2RAD,
                node->euler_degrees.y * DEG2RAD,
                node->euler_degrees.z * DEG2RAD
            );
        };

        ImGui::DragFloat3("Scale", reinterpret_cast<float*>(&node->scale), 0.1, 0);
        ImGui::EndGroup();
    }
} // namespace regan
