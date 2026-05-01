#pragma once
#include <string>
#include <string_view>
#include <vector>
#include <utility>
#include "parse/ast.h"

namespace latex_fmt {

    class SyntaxChecker {
    public:
        struct Error {
            int line;
            int col;
            std::string message;
        };

        SyntaxChecker(std::string_view source, const Document& ast)
            : source_(source), ast_(ast) {
            computeLineTable();
        }

        std::vector<Error> check() {
            errors_.clear();
            env_stack_.clear();
            walk(ast_);

            for (const auto& [name, offset] : env_stack_) {
                auto [line, col] = offsetToLineCol(offset);
                errors_.push_back({line, col,
                    "unclosed environment '\\begin{" + name + "}'"});
            }

            return errors_;
        }

    private:
        void computeLineTable() {
            line_starts_.push_back(0);
            for (size_t i = 0; i < source_.size(); ++i) {
                if (source_[i] == '\n') {
                    line_starts_.push_back(i + 1);
                }
            }
        }

        std::pair<int, int> offsetToLineCol(size_t offset) {
            if (offset >= source_.size()) offset = source_.size();
            int line = 1;
            int col = 1;
            for (size_t i = 0; i < line_starts_.size(); ++i) {
                if (i + 1 < line_starts_.size() && offset >= line_starts_[i + 1]) {
                    continue;
                }
                line = static_cast<int>(i + 1);
                col = static_cast<int>(offset - line_starts_[i] + 1);
                break;
            }
            return {line, col};
        }

        void walk(const ASTNode& node) {
            if (auto* doc = dynamic_cast<const Document*>(&node)) {
                for (const auto& child : doc->children) {
                    if (child) walk(*child);
                }
            } else if (auto* env = dynamic_cast<const Environment*>(&node)) {
                checkEnvironment(*env);
                env_stack_.push_back({env->name, env->source.begin_offset});
                for (const auto& child : env->children) {
                    if (child) walk(*child);
                }
                if (!env_stack_.empty() && env_stack_.back().first == env->name) {
                    env_stack_.pop_back();
                }
                if (env->is_malformed) {
                    auto [line, col] = offsetToLineCol(env->source.begin_offset);
                    errors_.push_back({line, col,
                        "environment '\\begin{" + env->name + "}' has no matching '\\end{" + env->name + "}'"});
                }
            } else if (auto* math = dynamic_cast<const InlineMath*>(&node)) {
                if (math->is_malformed) {
                    auto [line, col] = offsetToLineCol(math->source.begin_offset);
                    errors_.push_back({line, col,
                        "unclosed inline math delimiter '$'"});
                }
                for (const auto& child : math->children) {
                    if (child) walk(*child);
                }
            } else if (auto* math = dynamic_cast<const DisplayMath*>(&node)) {
                if (math->is_malformed) {
                    auto [line, col] = offsetToLineCol(math->source.begin_offset);
                    errors_.push_back({line, col,
                        "unclosed display math delimiter '$$'"});
                }
                for (const auto& child : math->children) {
                    if (child) walk(*child);
                }
            } else if (auto* group = dynamic_cast<const Group*>(&node)) {
                if (group->is_malformed) {
                    auto [line, col] = offsetToLineCol(group->source.begin_offset);
                    errors_.push_back({line, col,
                        "unclosed '" + group->delim_open + "'"});
                }
                for (const auto& child : group->children) {
                    if (child) walk(*child);
                }
            } else if (auto* cmd = dynamic_cast<const Command*>(&node)) {
                for (const auto& arg : cmd->args) {
                    if (arg) walk(*arg);
                }
            } else if (auto* txt = dynamic_cast<const Text*>(&node)) {
                if (txt->is_malformed) {
                    auto [line, col] = offsetToLineCol(txt->source.begin_offset);
                    errors_.push_back({line, col,
                        "extraneous '" + txt->content + "'"});
                }
            }
        }

        void checkEnvironment(const Environment& env) {
            for (const auto& [name, offset] : env_stack_) {
                if (name == env.name) {
                    auto [line, col] = offsetToLineCol(env.source.begin_offset);
                    errors_.push_back({line, col,
                        "nested duplicate environment '\\begin{" + env.name + "}'"});
                    break;
                }
            }
        }

        std::string_view source_;
        const Document& ast_;
        std::vector<size_t> line_starts_;
        std::vector<Error> errors_;
        std::vector<std::pair<std::string, size_t>> env_stack_;
    };

} // namespace latex_fmt
