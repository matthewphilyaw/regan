//
// Created by Matthew Philyaw on 6/3/26.
//

#pragma once
#include "raylib.h"

namespace regan::common {
    class ManagedModel {
    public:
        ManagedModel() = default;
        explicit ManagedModel(const Model &model): model_(model) {}
        ~ManagedModel() { UnloadModel(model_); }

        ManagedModel(const ManagedModel&) = delete;
        ManagedModel& operator=(const ManagedModel&) = delete;

        ManagedModel(ManagedModel&& other) noexcept : model_(other.model_) {
            other.model_ = {};
        }

        ManagedModel& operator=(ManagedModel&& other) noexcept {
            if (this != &other) {
                UnloadModel(model_);
                model_ = other.model_;
                other.model_ = {};
            }
            return *this;
        }

        Model& get() { return model_; }
        [[nodiscard]] const Model& get() const { return model_; }

        static ManagedModel take(Model& model) {
            auto m = ManagedModel(model);
            model = {};
            return m;
        }

    private:
        Model model_;
    };
} // regan