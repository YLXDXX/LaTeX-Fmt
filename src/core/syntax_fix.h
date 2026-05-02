#pragma once
#include <string>
#include <string_view>
#include <vector>
#include <algorithm>
#include "parse/ast.h"

namespace latex_fmt {

    class SyntaxFixer {
    public:
        struct Fix {
            size_t offset;
            std::string text;
        };

        SyntaxFixer(std::string_view source, const Document& ast)
            : source_(source) {
            walk(ast);
            std::sort(fixes_.begin(), fixes_.end(),
                [](const Fix& a, const Fix& b) { return a.offset > b.offset; });
        }

        std::string apply() const {
            std::string result(source_);
            size_t shift = 0;
            size_t prev_offset = std::string::npos;
            for (const auto& fix : fixes_) {
                if (fix.offset <= result.size()) {
                    if (fix.offset != prev_offset) {
                        shift = 0;
                        prev_offset = fix.offset;
                    }
                    result.insert(fix.offset + shift, fix.text);
                    shift += fix.text.size();
                }
            }
            return result;
        }

        const std::vector<Fix>& getFixes() const { return fixes_; }

    private:
        void walk(const ASTNode& node) {
            if (auto* doc = dynamic_cast<const Document*>(&node)) {
                for (const auto& child : doc->children) {
                    if (child) walk(*child);
                }
            } else if (auto* env = dynamic_cast<const Environment*>(&node)) {
                for (const auto& child : env->children) {
                    if (child) walk(*child);
                }
                if (env->is_malformed) {
                    fixes_.push_back({env->source.end_offset,
                        "\\end{" + env->name + "}"});
                }
            } else if (auto* math = dynamic_cast<const InlineMath*>(&node)) {
                for (const auto& child : math->children) {
                    if (child) walk(*child);
                }
                if (math->is_malformed) {
                    fixes_.push_back({math->source.end_offset, "$"});
                }
            } else if (auto* math = dynamic_cast<const DisplayMath*>(&node)) {
                for (const auto& child : math->children) {
                    if (child) walk(*child);
                }
                if (math->is_malformed) {
                    fixes_.push_back({math->source.end_offset, "$$"});
                }
            } else if (auto* group = dynamic_cast<const Group*>(&node)) {
                for (const auto& child : group->children) {
                    if (child) walk(*child);
                }
                if (group->is_malformed) {
                    size_t fix_offset = group->source.end_offset;
                    for (const auto& child : group->children) {
                        if (auto* inner = dynamic_cast<const Group*>(child.get())) {
                            if (inner->delim_open != group->delim_open && !inner->is_malformed) {
                                fix_offset = inner->source.begin_offset;
                                break;
                            }
                        }
                    }
                    fixes_.push_back({fix_offset, group->delim_close});
                }
            } else if (auto* cmd = dynamic_cast<const Command*>(&node)) {
                for (const auto& arg : cmd->args) {
                    if (arg) walk(*arg);
                }
            } else if (auto* txt = dynamic_cast<const Text*>(&node)) {
                if (txt->is_malformed) {
                    if (txt->content == "}") {
                        fixes_.push_back({txt->source.begin_offset, "{"});
                    } else if (txt->content == "]") {
                        fixes_.push_back({txt->source.begin_offset, "["});
                    }
                }
            }
        }

        std::string_view source_;
        std::vector<Fix> fixes_;
    };

} // namespace latex_fmt
