#pragma once

#include "editor/command.hpp"
#include "editor/entity.hpp"
#include "editor/registry.hpp"
#include "common/managed_model.hpp"
#include "common/managed_texture_2d.hpp"

namespace regan::editor::commands {
    class ChangeTextureCommand : public Command {
    public:
        ChangeTextureCommand(Registry<Entity> &entities,
                             Registry<common::ManagedModel> &models,
                             Registry<common::ManagedTexture2d> &textures,
                             size_t entity_id,
                             size_t old_tex_id,
                             size_t new_tex_id)
            : entities_(entities), models_(models), textures_(textures),
              entity_id_(entity_id),
              old_tex_id_(old_tex_id), new_tex_id_(new_tex_id) {
        }

        void execute() override { apply(new_tex_id_); }
        void undo() override { apply(old_tex_id_); }

    private:
        void apply(size_t tex_id) {
            auto *e = entities_.get(entity_id_);
            if (!e || !e->mesh || !e->mesh->model_id) return;

            e->mesh->texture_id = tex_id;

            auto *model = models_.get(e->mesh->model_id.value());
            auto *tex = textures_.get(tex_id);
            if (model && tex) {
                model->get().materials[0].maps[0].texture = tex->get();
            }
        }

        Registry<Entity> &entities_;
        Registry<common::ManagedModel> &models_;
        Registry<common::ManagedTexture2d> &textures_;
        size_t entity_id_;
        size_t old_tex_id_;
        size_t new_tex_id_;
    };
}
