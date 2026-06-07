#pragma once

#include <array>
#include <raylib.h>

namespace regan::editor {

    static constexpr int MAX_LIGHTS = 128;

    class LightSystem {
    public:
        explicit LightSystem(Shader& shader);
        ~LightSystem() = default;

        LightSystem(const LightSystem&)            = delete;
        LightSystem& operator=(const LightSystem&) = delete;
        LightSystem(LightSystem&&)                 = delete;
        LightSystem& operator=(LightSystem&&)      = delete;

        int  add_point_light(Vector3 pos, Vector3 color, float intensity,
                             Vector3 aabb_min, Vector3 aabb_max);
        void remove_light(int index);
        void clear();
        void upload();

        int count() const { return light_count_; }

    private:
        Shader& shader_;
        int     light_count_ = 0;

        // Parallel arrays, each element is a vec4 (4 floats)
        std::array<Vector4, MAX_LIGHTS> pos_intensity_{};
        std::array<Vector4, MAX_LIGHTS> color_{};
        std::array<Vector4, MAX_LIGHTS> aabb_min_{};
        std::array<Vector4, MAX_LIGHTS> aabb_max_{};

        // Cached uniform locations
        int pos_loc_       = -1;
        int color_loc_     = -1;
        int min_loc_       = -1;
        int max_loc_       = -1;
        int count_loc_     = -1;
    };

} // namespace regan::editor