//
// Created by Matthew Philyaw on 6/3/26.
//

#pragma once
#include "raylib.h"

namespace regan::common {
    class ManagedTexture2d {
    public:
        explicit ManagedTexture2d(Texture2D texture): texture_(texture) {}
        ~ManagedTexture2d() { UnloadTexture(texture_); }

        ManagedTexture2d(const ManagedTexture2d&) = delete;
        ManagedTexture2d& operator=(const ManagedTexture2d&) = delete;

        ManagedTexture2d(ManagedTexture2d&& other) noexcept : texture_(other.texture_) {
            other.texture_ = {};
        }

        ManagedTexture2d& operator=(ManagedTexture2d&& other) noexcept {
            if (this != &other) {
                UnloadTexture(texture_);
                texture_ = other.texture_;
                other.texture_ = {};
            }

            return *this;
        }

        Texture2D& get() { return texture_; }
        [[nodiscard]] const Texture2D& get() const { return texture_; }

    private:
        Texture2D texture_;
    };

}
