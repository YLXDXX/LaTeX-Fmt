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
            std::string context;
        };

        SyntaxChecker(std::string_view source, const Document& ast, int context_chars = 10)
            : source_(source), ast_(ast), context_chars_(context_chars) {
            computeLineTable();
        }

        std::vector<Error> check() {
            errors_.clear();
            env_stack_.clear();
            walk(ast_);

            for (const auto& [name, offset] : env_stack_) {
                auto [line, col] = offsetToLineCol(offset);
                makeError(line, col,
                    "unclosed environment '\\begin{" + name + "}'");
            }

            return errors_;
        }

        std::pair<int, int> getLineCol(size_t offset) const {
            return offsetToLineCol(offset);
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

        static int countChars(std::string_view s) {
            int n = 0;
            for (size_t i = 0; i < s.size(); ++i) {
                uint8_t b = static_cast<uint8_t>(s[i]);
                if ((b & 0xC0) != 0x80) ++n;
            }
            return n;
        }

        std::pair<int, int> offsetToLineCol(size_t offset) const {
            if (offset >= source_.size()) offset = source_.size();
            int line = 1;
            int col = 1;
            for (size_t i = 0; i < line_starts_.size(); ++i) {
                if (i + 1 < line_starts_.size() && offset >= line_starts_[i + 1]) {
                    continue;
                }
                line = static_cast<int>(i + 1);
                size_t line_start = line_starts_[i];
                col = countChars(source_.substr(line_start, offset - line_start)) + 1;
                break;
            }
            return {line, col};
        }

        std::string contextAt(int line, int col) const {
            if (line < 1 || (size_t)line > line_starts_.size()) return "";
            size_t line_start = line_starts_[static_cast<size_t>(line - 1)];
            size_t line_end = source_.size();
            if ((size_t)line < line_starts_.size()) {
                line_end = line_starts_[static_cast<size_t>(line)];
                if (line_end > 0 && source_[line_end - 1] == '\n') line_end--;
            }
            std::string_view line_str = source_.substr(line_start, line_end - line_start);

            size_t byte_offset = 0;
            int char_idx = 1;
            while (char_idx < col && byte_offset < line_str.size()) {
                uint8_t b = static_cast<uint8_t>(line_str[byte_offset]);
                byte_offset++;
                if ((b & 0xC0) != 0x80) char_idx++;
            }

            int ctx_before = context_chars_;
            size_t ctx_start = byte_offset;
            int seen = 0;
            while (seen < ctx_before && ctx_start > 0) {
                ctx_start--;
                uint8_t b = static_cast<uint8_t>(line_str[ctx_start]);
                if ((b & 0xC0) != 0x80) seen++;
            }

            int ctx_after = context_chars_;
            size_t ctx_end = byte_offset;
            seen = 0;
            while (seen < ctx_after && ctx_end < line_str.size()) {
                uint8_t b = static_cast<uint8_t>(line_str[ctx_end]);
                ctx_end++;
                if ((b & 0xC0) != 0x80) seen++;
            }
            while (ctx_end < line_str.size() && (static_cast<uint8_t>(line_str[ctx_end]) & 0xC0) == 0x80) {
                ctx_end++;
            }

            return std::string(line_str.substr(ctx_start, ctx_end - ctx_start));
        }

        void makeError(int line, int col, std::string msg) {
            errors_.push_back({line, col, std::move(msg), contextAt(line, col)});
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
                    makeError(line, col,
                        "environment '\\begin{" + env->name + "}' has no matching '\\end{" + env->name + "}'");
                }
            } else if (auto* math = dynamic_cast<const InlineMath*>(&node)) {
                if (math->is_malformed) {
                    auto [line, col] = offsetToLineCol(math->source.begin_offset);
                    makeError(line, col, "unclosed inline math delimiter '$'");
                }
                for (const auto& child : math->children) {
                    if (child) walk(*child);
                }
            } else if (auto* math = dynamic_cast<const DisplayMath*>(&node)) {
                if (math->is_malformed) {
                    auto [line, col] = offsetToLineCol(math->source.begin_offset);
                    makeError(line, col, "unclosed display math delimiter '$$'");
                }
                for (const auto& child : math->children) {
                    if (child) walk(*child);
                }
            } else if (auto* group = dynamic_cast<const Group*>(&node)) {
                if (group->is_malformed) {
                    auto [line, col] = offsetToLineCol(group->source.begin_offset);
                    makeError(line, col, "unclosed '" + group->delim_open + "'");
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
                    makeError(line, col, "extraneous '" + txt->content + "'");
                }
            }
        }

        void checkEnvironment(const Environment& env) {
            if (!env_stack_.empty() && env_stack_.back().first == env.name) {
                auto [line, col] = offsetToLineCol(env.source.begin_offset);
                makeError(line, col,
                    "nested duplicate environment '\\begin{" + env.name + "}'");
            }
        }

        std::string_view source_;
        const Document& ast_;
        std::vector<size_t> line_starts_;
        std::vector<Error> errors_;
        std::vector<std::pair<std::string, size_t>> env_stack_;
        int context_chars_ = 10;
    };

} // namespace latex_fmt
