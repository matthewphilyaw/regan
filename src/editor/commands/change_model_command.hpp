#pragma once

#include "editor/command.hpp"
#include "editor/entity.hpp"
#include "editor/registry.hpp"
#include "common/managed_model.hpp"
#include "common/managed_texture_2d.hpp"

namespace regan::editor::commands {
    class ChangeModelCommand : public Command {
    public:
        ChangeModelCommand(Registry<Entity> &entities,
                           Registry<common::ManagedModel> &models,
                           Registry<common::ManagedTexture2d> &textures,
                           size_t entity_id,
                           size_t old_model_id,
                           size_t new_model_id)
            : entities_(entities), models_(models), textures_(textures),
              entity_id_(entity_id),
              old_model_id_(old_model_id), new_model_id_(new_model_id) {
        }

        void execute() override { apply(new_model_id_); }
        void undo() override { apply(old_model_id_); }

    private:
        void apply(size_t model_id) {
            auto *e = entities_.get(entity_id_);
            if (!e || !e->mesh) return;

            e->mesh->model_id = model_id;

            // Rebind current texture onto the new model's material slot
            if (e->mesh->texture_id) {
                auto *model = models_.get(model_id);
                auto *tex = textures_.get(e->mesh->texture_id.value());
                if (model && tex) {
                    model->get().materials[0].maps[0].texture = tex->get();
                }
            }
        }

        Registry<Entity> &entities_;
        Registry<common::ManagedModel> &models_;
        Registry<common::ManagedTexture2d> &textures_;
        size_t entity_id_;
        size_t old_model_id_;
        size_t new_model_id_;
    };
}
