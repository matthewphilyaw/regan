//
// Created by Matthew Philyaw on 6/1/26.
//

#pragma once
#include <map>

namespace regan::editor {
    template<typename T>
    class Registry {
    public:
        size_t add(T item) {
            auto id = ++last_id_;
            this->registry_[id] = std::move(item);
            return id;
        }

        T* get(size_t id) {
            auto it = registry_.find(id);
            if (it == registry_.end()) {
                return nullptr;
            }

            return &it->second;
        }

        void remove(size_t id) {
            registry_.erase(id);
        }

        size_t size() {
            return registry_.size();
        }

        bool empty() {
            return registry_.empty();
        }

        auto begin() const { return registry_.begin(); }
        auto end()   const { return registry_.end(); }

    private:
        size_t last_id_{};
        std::map<size_t, T> registry_{};
    };
}
