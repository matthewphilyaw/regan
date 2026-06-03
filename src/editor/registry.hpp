//
// Created by Matthew Philyaw on 6/1/26.
//

#pragma once
#include <unordered_map>

namespace regan::editor {
    template<typename T>
    class Registry {
    public:
        size_t add(T item) {
            auto id = ++this->last_id;
            this->registry[id] = std::move(item);
            return id;
        }

        T* get(size_t id) {
            auto it = this->registry.find(id);
            if (it == this->registry.end()) {
                return nullptr;
            }

            return &it->second;
        }

        void remove(size_t id) {
            this->registry.erase(id);
        }

        auto begin() const { return registry.begin(); }
        auto end()   const { return registry.end(); }

    private:
        size_t last_id{};
        std::unordered_map<size_t, T> registry{};
    };
}
