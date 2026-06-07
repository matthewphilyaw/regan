//
// Created by Matthew Philyaw on 6/6/26.
//

#pragma once
#include "editor/command.hpp"
#include "editor/entity.hpp"
#include "editor/registry.hpp"

namespace regan::editor::commands {
    class AddEntityCommand : public Command {
    public:
        AddEntityCommand(Registry<Entity> &reg, Entity entity)
            : registry_(reg), entity_(std::move(entity)) {
        }

        void execute() override {
            if (id_ == 0) {
                id_ = registry_.add(entity_);          // first run, generate
            } else {
                registry_.insert(id_, entity_);        // redo, restore original id
            }
        }

        void undo() override {
            registry_.remove(id_);
        }

    private:
        Registry<Entity> &registry_;
        Entity entity_;
        size_t id_ = 0;
    };
}
