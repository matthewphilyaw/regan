#include "editor.hpp"

#include <cmath>

#include "imgui.h"
#include "raylib.h"
#include "raymath.h"
#include "rlImGui.h"
#include "ImGuizmo.h"
#include "engine/engine.hpp"
#include <filesystem>
#include <ranges>

#include "ImGuiFileDialog.h"
#include "rlgl.h"
#include "commands/add_entity_command.hpp"
#include "commands/change_model_command.hpp"
#include "commands/change_texture_command.hpp"
#include "commands/remove_entity_command.hpp"
#include "common/managed_model.hpp"
#include "common/managed_texture_2d.hpp"

namespace fs = std::filesystem;


namespace regan::editor {
    fs::path asset_path = "/Users/mphilyaw/code/regan/assets";

    static Matrix matrix_from_floats(const float f[16]) {
        return Matrix{
            f[0], f[1], f[2], f[3],
            f[4], f[5], f[6], f[7],
            f[8], f[9], f[10], f[11],
            f[12], f[13], f[14], f[15]
        };
    }

    inline Vector3 light_half_extents(const Entity &e) {
        return {
            0.5f * e.transform.scale.x,
            0.5f * e.transform.scale.y,
            0.5f * e.transform.scale.z
        };
    }

    static Matrix entity_transform_matrix(const EntityTransform &t) {
        return MatrixMultiply(
            MatrixMultiply(
                MatrixScale(t.scale.x, t.scale.y, t.scale.z),
                QuaternionToMatrix(t.rotation)
            ),
            MatrixTranslate(t.position.x, t.position.y, t.position.z)
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

    Editor::Editor(Engine &engine) : engine_(engine),
                                     lighting_shader_(LoadShader(
                                         (asset_path / "shaders" / "lighting.vert").c_str(),
                                         (asset_path / "shaders" / "lighting.frag").c_str())),
                                     light_system_(lighting_shader_) {
        EnableCursor();
        SetMouseCursor(MOUSE_CURSOR_CROSSHAIR);
        rlImGuiSetup(true);
        editor_camera_.update_camera_position(camera_3d_);

        uniform_locs_.lightCount = GetShaderLocation(lighting_shader_, "lightCount");
        uniform_locs_.ambient = GetShaderLocation(lighting_shader_, "ambient");
        uniform_locs_.edgeFade = GetShaderLocation(lighting_shader_, "edgeFade");
        uniform_locs_.matModel = GetShaderLocation(lighting_shader_, "matModel");
        uniform_locs_.matNormal = GetShaderLocation(lighting_shader_, "matNormal");
        mat_model_loc_ = GetShaderLocation(lighting_shader_, "matModel");
        mat_normal_loc_ = GetShaderLocation(lighting_shader_, "matNormal");
        ambient_loc_ = GetShaderLocation(lighting_shader_, "ambient");
        edge_fade_loc_ = GetShaderLocation(lighting_shader_, "edgeFade");
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
        if (ImGuizmo::IsUsing()) return;

        Ray ray = GetScreenToWorldRay(GetMousePosition(), camera_3d_);

        std::optional<size_t> hit_index;
        float closest = FLT_MAX;

        for (auto &[id, entity]: entities_) {
            if (entity.light.has_value()) {
                const Vector3 p = entity.transform.position;
                RayCollision hit = GetRayCollisionSphere(ray, p, 0.4f);
                if (hit.hit && hit.distance < closest) {
                    closest = hit.distance;
                    hit_index = id;
                }
                continue;
            }

            if (!entity.mesh.has_value() || !entity.mesh.value().model_id.has_value()) {
                continue;
            }

            auto *managed_model = models_.get(entity.mesh.value().model_id.value());
            if (!managed_model) continue;

            const auto transform = entity_transform_matrix(entity.transform);

            const auto &model = managed_model->get();
            RayCollision hit = GetRayCollisionMesh(ray, model.meshes[0], transform);
            if (hit.hit && hit.distance < closest) {
                closest = hit.distance;
                hit_index = id;
            }
        }

        selected_ = hit_index;
    }

    void Editor::update() {
        update_camera();
        update_shortcuts();

        if (selected_.has_value() && !entities_.get(selected_.value())) {
            selected_.reset();
        }

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

        auto entity = entities_.get(selected_.value());
        if (entity == nullptr) {
            return;
        }

        if (!entity->mesh.has_value() || !entity->mesh.value().model_id.has_value()) {
            return;
        }

        auto *model = models_.get(entity->mesh.value().model_id.value());
        if (!model) return;

        BoundingBox box = GetModelBoundingBox(model->get());
        Matrix transform = entity_transform_matrix(entity->transform);

        rlPushMatrix();
        rlMultMatrixf(MatrixToFloat(transform));
        DrawBoundingBox(box, MAGENTA);
        rlPopMatrix();
    }

    void Editor::draw_light_gizmos() {
        for (const auto &entity: entities_ | std::views::values) {
            if (!entity.light.has_value()) continue;

            const auto &l = entity.light.value();
            const Vector3 p = entity.transform.position;
            const auto [x, y, z] = light_half_extents(entity);

            // Color the wireframe to match the light, but dimmed
            Color box_color = {
                (unsigned char) (l.color.x * 255),
                (unsigned char) (l.color.y * 255),
                (unsigned char) (l.color.z * 255),
                128
            };

            BoundingBox box = {
                {p.x - x, p.y - y, p.z - z},
                {p.x + x, p.y + y, p.z + z}
            };
            DrawBoundingBox(box, box_color);

            // Center marker — small sphere at the light source
            DrawSphere(p, 0.4f, box_color);
        }
    }

    void Editor::draw_gizmo() {
        ImGuizmo::SetOrthographic(false);
        ImGuizmo::BeginFrame();
        ImGuizmo::SetRect(0, 0, GetScreenWidth(), GetScreenHeight());
        if (selected_.has_value()) {
            auto entity = entities_.get(selected_.value());
            if (entity == nullptr) {
                return;
            }

            auto transform = entity_transform_matrix(entity->transform);

            Matrix view = GetCameraMatrix(camera_3d_);
            Matrix projection = MatrixPerspective(
                camera_3d_.fovy * DEG2RAD,
                (float) GetScreenWidth() / GetScreenHeight(),
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
                entity->transform.rotation = QuaternionNormalize(
                    QuaternionMultiply(delta_q, entity->transform.rotation));

                // update euler for display
                Vector3 e = QuaternionToEuler(entity->transform.rotation);
                entity->transform.euler_degrees = {e.x * RAD2DEG, e.y * RAD2DEG, e.z * RAD2DEG};

                // position and scale still from decompose
                float t[3], r[3], s[3];
                ImGuizmo::DecomposeMatrixToComponents(transform_f, t, r, s);
                entity->transform.position = {t[0], t[1], t[2]};
                entity->transform.scale = {s[0], s[1], s[2]};
            }
        }
    }

    void Editor::draw_grid() {
        DrawGrid(100, 1);
        DrawLine3D({0, 0, 0}, {1, 0, 0}, RED);
        DrawLine3D({0, 0, 0}, {0, 1, 0}, GREEN);
        DrawLine3D({0, 0, 0}, {0, 0, 1}, BLUE);
    }

    void Editor::draw_entities() {
        light_system_.clear();

        for (const auto &entity: entities_ | std::views::values) {
            if (!entity.light.has_value()) continue;

            const auto &l = entity.light.value();
            const Vector3 p = entity.transform.position;
            const auto [x, y, z] = light_half_extents(entity);

            light_system_.add_point_light(
                p, l.color, l.intensity,
                {p.x - x, p.y - y, p.z - z},
                {p.x + x, p.y + y, p.z + z}
            );
        }

        light_system_.upload();

        Vector3 ambient = {0.15f, 0.15f, 0.15f};
        SetShaderValue(lighting_shader_, ambient_loc_, &ambient, SHADER_UNIFORM_VEC3);
        float edge_fade = 0.5f;
        SetShaderValue(lighting_shader_, edge_fade_loc_, &edge_fade, SHADER_UNIFORM_FLOAT);

        for (const auto &entity: entities_ | std::views::values) {
            if (!entity.mesh.has_value()) continue;

            auto *model = models_.get(entity.mesh.value().model_id.value());
            if (model == nullptr) continue;

            Matrix mat_model = entity_transform_matrix(entity.transform);
            Matrix mat_normal = MatrixTranspose(MatrixInvert(mat_model));

            SetShaderValueMatrix(lighting_shader_, mat_model_loc_, mat_model);
            SetShaderValueMatrix(lighting_shader_, mat_normal_loc_, mat_normal);

            for (int m = 0; m < model->get().meshCount; m++) {
                model->get().materials[m].shader = lighting_shader_;
                DrawMesh(model->get().meshes[m], model->get().materials[m], mat_model);
            }
        }
    }

    void Editor::draw() {
        ClearBackground({20, 20, 20, 255});

        BeginMode3D(camera_3d_);

        draw_grid();
        draw_entities();
        draw_light_gizmos();
        draw_selection_highlight();

        EndMode3D();

        rlImGuiBegin();

        gui_draw_menu_bar();
        gui_draw_perf_overlay();
        gui_draw_outliner_panel();
        gui_draw_entity_properties();
        gui_draw_add_model_popup();
        gui_draw_change_model_dialog();
        gui_draw_change_texture_dialog();
        draw_gizmo();

        rlImGuiEnd();
    }

    void Editor::update_camera() {
        static bool middle_dragging = false;

        constexpr float sensitivity = 0.005f;
        constexpr float pan_speed = 0.005f;
        constexpr float zoom_speed = 0.5f;

        bool middle_pressed = IsMouseButtonPressed(MOUSE_MIDDLE_BUTTON);
        bool middle_released = IsMouseButtonReleased(MOUSE_MIDDLE_BUTTON);
        bool shift_down = IsKeyDown(KEY_LEFT_SHIFT);

        // Start a drag only when imgui doesn't want the mouse
        if (middle_pressed && !ImGui::GetIO().WantCaptureMouse) {
            middle_dragging = true;
            lock_cursor();
        }

        if (middle_released) {
            if (middle_dragging) unlock_cursor();
            middle_dragging = false;
        }

        // Once dragging, ignore WantCaptureMouse — finish the gesture
        if (middle_dragging) {
            Vector2 delta = GetMouseDelta();

            if (!shift_down) {
                editor_camera_.yaw -= delta.x * sensitivity;
                editor_camera_.pitch += delta.y * sensitivity;
                editor_camera_.pitch = Clamp(editor_camera_.pitch, -1.5f, 1.5f);
                editor_camera_.update_camera_position(camera_3d_);
            } else {
                Vector3 forward = Vector3Normalize(
                    Vector3Subtract(camera_3d_.target, camera_3d_.position)
                );
                Vector3 right = Vector3Normalize(
                    Vector3CrossProduct(forward, {0.0f, 1.0f, 0.0f})
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

        // Scroll — guard separately, hover-matters for scroll
        if (!ImGui::GetIO().WantCaptureMouse) {
            float scroll = GetMouseWheelMove();
            if (scroll != 0.0f) {
                editor_camera_.distance -= scroll * zoom_speed;
                editor_camera_.distance = Clamp(editor_camera_.distance, 0.5f, 10000.0f);
                editor_camera_.update_camera_position(camera_3d_);
            }
        }
    }

    void Editor::gui_draw_menu_bar() {
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("Exit to Menu")) {
                    engine_.request_transition_to_state(EngineState::MainMenu);
                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Add")) {
                if (ImGui::MenuItem("Light")) {
                    add_light_entity();
                }
                if (ImGui::MenuItem("Model...")) {
                    pending_model_path_.clear();
                    pending_texture_path_.clear();
                    show_add_model_popup_ = true;
                }
                ImGui::EndMenu();
            }


            ImGui::EndMainMenuBar();
        }
    }

    void Editor::gui_draw_outliner_panel() {
        ImGui::SetNextWindowSize(ImVec2(300, 600), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos(ImVec2(0, 20), ImGuiCond_FirstUseEver);
        ImGui::Begin("Outline");

        if (ImGui::BeginListBox("##outliner", ImVec2(-1, -1))) {
            std::optional<size_t> to_remove;
            std::optional<size_t> to_duplicate;

            for (auto &[id, entity]: entities_) {
                ImGui::PushID(static_cast<int>(id));
                bool current_selection = selected_.has_value() && selected_.value() == id;
                if (ImGui::Selectable(entity.name.c_str(), current_selection)) {
                    selected_ = id;
                }

                if (ImGui::BeginPopupContextItem()) {
                    if (ImGui::MenuItem("Delete")) {
                        to_remove = id;
                    }
                    if (ImGui::MenuItem("Duplicate")) {
                        to_duplicate = id;
                    }
                    ImGui::EndPopup();
                }
                ImGui::PopID();
            }

            if (to_remove.has_value()) {
                remove_entity(to_remove.value());
            }

            if (to_duplicate.has_value()) {
                duplicate_entity(to_duplicate.value());
            }

            ImGui::EndListBox();
        }

        ImGui::End();
    }

    void Editor::gui_draw_entity_properties() {
        ImVec2 screen = ImGui::GetIO().DisplaySize;
        float panel_width = 300.0f;

        ImGui::SetNextWindowSize(ImVec2(panel_width, 600), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos(ImVec2(screen.x - panel_width, 20), ImGuiCond_FirstUseEver);
        ImGui::Begin("Node Properties");

        if (!selected_.has_value() || entities_.empty()) {
            ImGui::End();
            return;
        }

        if (ImGui::Button("Delete Entity")) {
            remove_entity(selected_.value());
            ImGui::End();
            return;
        }

        ImGui::Separator();

        const auto entity = entities_.get(selected_.value());
        gui_draw_entity_transform_properties(*entity);

        if (entity->mesh.has_value()) {
            ImGui::SeparatorText("Mesh");
            if (ImGui::Button("Change Model...")) {
                pending_swap_entity_id_ = selected_.value();
                show_change_model_dialog_ = true;
            }
            ImGui::SameLine();
            if (ImGui::Button("Change Texture...")) {
                pending_swap_entity_id_ = selected_.value();
                show_change_texture_dialog_ = true;
            }
        }

        if (entity->light.has_value()) {
            gui_draw_entity_light_properties(*entity);
        }

        ImGui::End();
    }

    void Editor::gui_draw_entity_transform_properties(Entity &entity) {
        if (!selected_.has_value()) {
            return;
        }

        EntityTransform &t = entity.transform;
        ImGui::BeginGroup();
        ImGui::DragFloat3("Position", reinterpret_cast<float *>(&t.position), 0.1f);
        if (ImGui::DragFloat3("Rotation", &t.euler_degrees.x, 0.1f)) {
            t.rotation = QuaternionFromEuler(
                t.euler_degrees.x * DEG2RAD,
                t.euler_degrees.y * DEG2RAD,
                t.euler_degrees.z * DEG2RAD
            );
        };

        ImGui::DragFloat3("Scale", reinterpret_cast<float *>(&t.scale), 0.1f, 0);
        ImGui::EndGroup();
    }

    void Editor::gui_draw_entity_light_properties(Entity &entity) {
        if (!entity.light.has_value()) {
            return;
        }

        EntityLight &l = entity.light.value();

        ImGui::SeparatorText("Light");
        ImGui::BeginGroup();

        ImGui::ColorEdit3("Color", reinterpret_cast<float *>(&l.color));
        ImGui::DragFloat("Intensity", &l.intensity, 0.05f, 0.0f, 100.0f);
        ImGui::EndGroup();
    }

    void Editor::gui_draw_perf_overlay() {
        ImGui::SetNextWindowPos(ImVec2(10, 30), ImGuiCond_FirstUseEver);
        ImGui::Begin("Perf", nullptr,
                     ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize);

        float frame_ms = GetFrameTime() * 1000.0f;
        ImGui::Text("%.2f ms (%.0f FPS)", frame_ms, 1000.0f / frame_ms);

        static float history[120] = {};
        static int idx = 0;
        history[idx] = frame_ms;
        idx = (idx + 1) % 120;
        ImGui::PlotLines("##frametime", history, 120, idx,
                         nullptr, 0.0f, 33.0f, ImVec2(200, 50));

        ImGui::End();
    }

    void Editor::add_light_entity() {
        Entity e;
        e.name = "Light";
        e.transform.position = camera_3d_.target; // place at look-target so it's visible
        e.transform.scale = {5.0f, 4.0f, 5.0f};
        e.light = EntityLight{
            .color = {1.0f, 0.9f, 0.8f},
            .intensity = 2.0f,
        };

        history_.execute(std::make_unique<commands::AddEntityCommand>(
            entities_, std::move(e)));
    }

    void Editor::add_model_entity(std::string model_path, std::string texture_path) {
        auto model_fqp = (asset_path / model_path);
        auto tex_fqp = (asset_path / texture_path);

        size_t model_id = models_.add(common::ManagedModel(LoadModel(model_fqp.c_str())));
        size_t tex_id = textures_.add(common::ManagedTexture2d(LoadTexture(tex_fqp.c_str())));

        auto model = models_.get(model_id);
        model->get().materials[0].maps[MATERIAL_MAP_ALBEDO].texture = textures_.get(tex_id)->get();

        Entity e{
            .name = model_fqp.filename().string(),
            .transform = {
                .position = {0, 0, 0},
                .euler_degrees = {0, 0, 0},
                .rotation = QuaternionIdentity(),
                .scale = {1, 1, 1},
            },
            .mesh = EntityMesh{
                .model_id = model_id,
                .texture_id = tex_id,
            },
        };

        history_.execute(std::make_unique<commands::AddEntityCommand>(entities_, std::move(e)));
    }

    void Editor::remove_entity(size_t id) {
        history_.execute(std::make_unique<commands::RemoveEntityCommand>(
            entities_, id));

        if (selected_.has_value() && selected_.value() == id) {
            selected_.reset();
        }
    }

    void Editor::update_shortcuts() {
        if (ImGui::GetIO().WantCaptureKeyboard) return;

        bool mod = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL) ||
                   IsKeyDown(KEY_LEFT_SUPER) || IsKeyDown(KEY_RIGHT_SUPER);

        if (!mod) return;

        bool shift = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);

        if (IsKeyPressed(KEY_Z)) {
            if (shift) {
                history_.redo(); // Cmd/Ctrl+Shift+Z
            } else {
                history_.undo(); // Cmd/Ctrl+Z
            }
        }

        if (IsKeyPressed(KEY_Y)) {
            history_.redo(); // Cmd/Ctrl+Y (Windows convention)
        }
    }

    void Editor::gui_draw_add_model_popup() {
        if (!show_add_model_popup_) return;

        ImGui::OpenPopup("Add Model");
        if (ImGui::BeginPopupModal("Add Model", &show_add_model_popup_,
                                   ImGuiWindowFlags_AlwaysAutoResize)) {
            if (ImGui::Button("Model...")) {
                IGFD::FileDialogConfig cfg;
                cfg.path = (asset_path / "kit").string();
                ImGuiFileDialog::Instance()->OpenDialog(
                    "PickModel", "Choose Model", ".glb,.gltf,.obj", cfg);
            }
            ImGui::SameLine();
            ImGui::TextUnformatted(
                pending_model_path_.empty() ? "(none)" : pending_model_path_.c_str());

            if (ImGui::Button("Texture...")) {
                IGFD::FileDialogConfig cfg;
                cfg.path = (asset_path / "textures").string();
                ImGuiFileDialog::Instance()->OpenDialog(
                    "PickTexture", "Choose Texture", ".png,.jpg,.jpeg", cfg);
            }
            ImGui::SameLine();
            ImGui::TextUnformatted(
                pending_texture_path_.empty() ? "(none)" : pending_texture_path_.c_str());

            if (ImGuiFileDialog::Instance()->Display("PickModel")) {
                if (ImGuiFileDialog::Instance()->IsOk()) {
                    pending_model_path_ = ImGuiFileDialog::Instance()->GetFilePathName();
                }
                ImGuiFileDialog::Instance()->Close();
            }
            if (ImGuiFileDialog::Instance()->Display("PickTexture")) {
                if (ImGuiFileDialog::Instance()->IsOk()) {
                    pending_texture_path_ = ImGuiFileDialog::Instance()->GetFilePathName();
                }
                ImGuiFileDialog::Instance()->Close();
            }

            ImGui::Separator();
            bool can_add = !pending_model_path_.empty() && !pending_texture_path_.empty();
            if (!can_add) ImGui::BeginDisabled();
            if (ImGui::Button("Add", ImVec2(120, 0))) {
                add_model_entity(pending_model_path_, pending_texture_path_);
                pending_model_path_.clear();
                pending_texture_path_.clear();
                show_add_model_popup_ = false;
            }
            if (!can_add) ImGui::EndDisabled();
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(120, 0))) {
                pending_model_path_.clear();
                pending_texture_path_.clear();
                show_add_model_popup_ = false;
            }
            ImGui::EndPopup();
        }
    }

    void Editor::duplicate_entity(size_t id) {
        auto *src = entities_.get(id);
        if (!src) return;

        Entity copy = *src;
        copy.name += " (copy)";
        copy.transform.position.x += 1.0f;

        history_.execute(std::make_unique<commands::AddEntityCommand>(
            entities_, std::move(copy)));
    }

    // editor.cpp

    void Editor::gui_draw_change_model_dialog() {
        if (!show_change_model_dialog_) return;

        if (!ImGuiFileDialog::Instance()->IsOpened("ChangeModel")) {
            IGFD::FileDialogConfig cfg;
            cfg.path = (asset_path / "kit").string();
            ImGuiFileDialog::Instance()->OpenDialog(
                "ChangeModel", "Change Model", ".glb,.gltf,.obj", cfg);
        }

        if (ImGuiFileDialog::Instance()->Display("ChangeModel")) {
            if (ImGuiFileDialog::Instance()->IsOk()) {
                std::string path = ImGuiFileDialog::Instance()->GetFilePathName();
                change_entity_model(pending_swap_entity_id_, path);
            }
            ImGuiFileDialog::Instance()->Close();
            show_change_model_dialog_ = false;
        }
    }

    void Editor::gui_draw_change_texture_dialog() {
        if (!show_change_texture_dialog_) return;

        if (!ImGuiFileDialog::Instance()->IsOpened("ChangeTexture")) {
            IGFD::FileDialogConfig cfg;
            cfg.path = (asset_path / "textures").string();
            ImGuiFileDialog::Instance()->OpenDialog(
                "ChangeTexture", "Change Texture", ".png,.jpg,.jpeg", cfg);
        }

        if (ImGuiFileDialog::Instance()->Display("ChangeTexture")) {
            if (ImGuiFileDialog::Instance()->IsOk()) {
                std::string path = ImGuiFileDialog::Instance()->GetFilePathName();
                change_entity_texture(pending_swap_entity_id_, path);
            }
            ImGuiFileDialog::Instance()->Close();
            show_change_texture_dialog_ = false;
        }
    }

    void Editor::change_entity_model(size_t entity_id, const std::string &path) {
        auto *e = entities_.get(entity_id);
        if (!e || !e->mesh || !e->mesh->model_id) return;

        size_t old_id = e->mesh->model_id.value();
        size_t new_id = models_.add(common::ManagedModel(LoadModel(path.c_str())));

        history_.execute(std::make_unique<commands::ChangeModelCommand>(
            entities_, models_, textures_, entity_id, old_id, new_id));
    }

    void Editor::change_entity_texture(size_t entity_id, const std::string &path) {
        auto *e = entities_.get(entity_id);
        if (!e || !e->mesh || !e->mesh->texture_id) return;

        size_t old_id = e->mesh->texture_id.value();
        size_t new_id = textures_.add(common::ManagedTexture2d(LoadTexture(path.c_str())));

        history_.execute(std::make_unique<commands::ChangeTextureCommand>(
            entities_, models_, textures_, entity_id, old_id, new_id));
    }
} // namespace regan
