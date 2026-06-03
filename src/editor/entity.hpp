//
// Created by Matthew Philyaw on 6/2/26.
//

#pragma once

#include <string>
#include "raylib.h"
#include "raymath.h"

namespace regan::editor {
    struct EntityTransform {
        Vector3 position{};
        Vector3 euler_degrees{};
        Quaternion rotation = QuaternionIdentity();
        Vector3 scale = {1.0, 1.0, 1.0};
    };

    struct Entity {
        size_t id;
        std::string name;
        EntityTransform transform{};

    };
}
