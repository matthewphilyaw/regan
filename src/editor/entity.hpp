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
    };
}
