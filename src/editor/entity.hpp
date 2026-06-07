//
// Created by Matthew Philyaw on 6/2/26.
//

#pragma once

#include <string>
#include <optional>
#include "raylib.h"
#include "raymath.h"

namespace regan::editor {
    struct EntityMesh {
        std::optional<size_t> model_id{};
        std::optional<size_t> texture_id{};
    };

    struct EntityLight {
        Vector3 color = {1.0f, 1.0f, 1.0f};
        float   intensity = 1.0f;
        Vector3 half_extents = {2.5f, 2.5f, 2.5f};  // per-axis range, world units
    };

    struct EntityTransform {
        Vector3 position{};
        Vector3 euler_degrees{};
        Quaternion rotation = QuaternionIdentity();
        Vector3 scale = {1.0, 1.0, 1.0};
    };

    struct Entity {
        std::string name;
        EntityTransform transform{};
        std::optional<EntityMesh> mesh{};
        std::optional<EntityLight> light{};
    };
}