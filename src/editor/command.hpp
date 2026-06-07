#pragma once
#include <memory>
#include <vector>

namespace regan::editor {
    class Command {
    public:
        virtual ~Command() = default;
        virtual void execute() = 0;
        virtual void undo() = 0;
    };

    class CommandHistory {
    public:
        void execute(std::unique_ptr<Command> cmd) {
            cmd->execute();
            undo_stack_.push_back(std::move(cmd));
            redo_stack_.clear(); // new action invalidates redo
        }

        void undo() {
            if (undo_stack_.empty()) return;
            auto cmd = std::move(undo_stack_.back());
            undo_stack_.pop_back();
            cmd->undo();
            redo_stack_.push_back(std::move(cmd));
        }

        void redo() {
            if (redo_stack_.empty()) return;
            auto cmd = std::move(redo_stack_.back());
            redo_stack_.pop_back();
            cmd->execute();
            undo_stack_.push_back(std::move(cmd));
        }

        bool can_undo() const { return !undo_stack_.empty(); }
        bool can_redo() const { return !redo_stack_.empty(); }

    private:
        std::vector<std::unique_ptr<Command> > undo_stack_;
        std::vector<std::unique_ptr<Command> > redo_stack_;
    };
}
