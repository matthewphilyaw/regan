#pragma once
#include "editor/command.hpp"
#include "editor/entity.hpp"
#include "editor/registry.hpp"

namespace regan::editor::commands {
    class RemoveEntityCommand : public Command {
    public:
        RemoveEntityCommand(Registry<Entity>& reg, size_t id)
            : registry_(reg), id_(id) {}

        void execute() override {
            auto* e = registry_.get(id_);
            if (!e) return;
            saved_ = *e;
            registry_.remove(id_);
        }
        void undo() override {
            registry_.insert(id_, saved_);  // restore at original id
        }

    private:
        Registry<Entity>& registry_;
        size_t id_;
        Entity saved_{};
    };
}
