//
// Created by Matthew Philyaw on 6/6/26.
//

#include "light_system.hpp"

namespace regan::editor {

    LightSystem::LightSystem(Shader &shader)
        : shader_   (shader)
        , pos_loc_  (GetShaderLocation(shader, "lightPosIntensity"))
        , color_loc_(GetShaderLocation(shader, "lightColor"))
        , min_loc_  (GetShaderLocation(shader, "lightAabbMin"))
        , max_loc_  (GetShaderLocation(shader, "lightAabbMax"))
        , count_loc_(GetShaderLocation(shader, "lightCount"))
    {
    }

    void LightSystem::upload() {
        if (light_count_ > 0) {
            SetShaderValueV(shader_, pos_loc_,   pos_intensity_.data(), SHADER_UNIFORM_VEC4, light_count_);
            SetShaderValueV(shader_, color_loc_, color_.data(),         SHADER_UNIFORM_VEC4, light_count_);
            SetShaderValueV(shader_, min_loc_,   aabb_min_.data(),      SHADER_UNIFORM_VEC4, light_count_);
            SetShaderValueV(shader_, max_loc_,   aabb_max_.data(),      SHADER_UNIFORM_VEC4, light_count_);
        }

        SetShaderValue(shader_, count_loc_, &light_count_, SHADER_UNIFORM_INT);
    }

    int LightSystem::add_point_light(Vector3 pos, Vector3 color, float intensity,
                                     Vector3 aabb_min, Vector3 aabb_max) {
        if (light_count_ >= MAX_LIGHTS) return -1;

        int i = light_count_++;

        pos_intensity_[i] = { pos.x, pos.y, pos.z, intensity };
        color_[i]         = { color.x, color.y, color.z, 0.0f };
        aabb_min_[i]      = { aabb_min.x, aabb_min.y, aabb_min.z, 0.0f };
        aabb_max_[i]      = { aabb_max.x, aabb_max.y, aabb_max.z, 0.0f };

        return i;
    }

    void LightSystem::remove_light(int index) {
        if (index < 0 || index >= light_count_) return;

        int last = --light_count_;
        pos_intensity_[index] = pos_intensity_[last];
        color_[index]         = color_[last];
        aabb_min_[index]      = aabb_min_[last];
        aabb_max_[index]      = aabb_max_[last];
    }

    void LightSystem::clear() {
        light_count_ = 0;
    }

} // namespace regan::editor