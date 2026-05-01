#pragma once
#include <string>
#include <string_view>
#include <vector>
#include "core/source_location.h"
#include "core/registry.h"

namespace latex_fmt {

    enum class LexMode {
        Normal,
        VerbatimSeekEnd,
        VerbSeekDelim
    };

    enum class TokenType {
        Command,
        Text,
        Comment,
        BeginEnv,
        EndEnv,
        OpenBrace,
        CloseBrace,
        OpenBracket,
        CloseBracket,
        ParBreak,
        VerbContent,
        VerbatimContent,
        Newline,
        InlineMathStart,
        InlineMathEnd,
        DisplayMathStart,
        DisplayMathEnd,
        Eof
    };

    struct Token {
        TokenType type;
        std::string value;
        SourceRange source;
        bool is_line_end = false;
    };

    class Lexer {
    public:
        explicit Lexer(std::string_view input, const Registry& registry)
        : input_(input), pos_(0), registry_(registry) {}

        std::vector<Token> tokenize() {
            std::vector<Token> tokens;
            while (pos_ < input_.size()) {
                Token tok = nextToken();
                if (tok.type != TokenType::Eof) {
                    tokens.push_back(std::move(tok));
                }
            }
            tokens.push_back({TokenType::Eof, "", {pos_, pos_}});
            return tokens;
        }

    private:
        std::string_view input_;
        size_t pos_;
        LexMode mode_ = LexMode::Normal;
        std::string verbatim_end_tag_;
        const Registry& registry_;
        int math_mode_depth_ = 0;
        int display_math_depth_ = 0;

        char peek(size_t offset = 0) const {
            if (pos_ + offset < input_.size()) return input_[pos_ + offset];
            return '\0';
        }

        char consume() {
            return input_[pos_++];
        }

        void skipWhitespace() {
            while (pos_ < input_.size() && (input_[pos_] == ' ' || input_[pos_] == '\t')) {
                pos_++;
            }
        }

        Token nextToken() {
            if (mode_ == LexMode::VerbatimSeekEnd) return lexVerbatimSeekEnd();
            if (mode_ == LexMode::VerbSeekDelim) return lexVerbSeekDelim();

            size_t start = pos_;
            char c = peek();

            if (c == '$') {
                if (peek(1) == '$') {
                    pos_ += 2;
                    if (display_math_depth_ == 0) {
                        display_math_depth_ = 1;
                        return {TokenType::DisplayMathStart, "$$", {start, pos_}};
                    } else {
                        display_math_depth_ = 0;
                        return {TokenType::DisplayMathEnd, "$$", {start, pos_}};
                    }
                }
                pos_++;
                if (math_mode_depth_ == 0) {
                    math_mode_depth_ = 1;
                    return {TokenType::InlineMathStart, "$", {start, pos_}};
                } else {
                    math_mode_depth_ = 0;
                    return {TokenType::InlineMathEnd, "$", {start, pos_}};
                }
            }

            if (c == '\n' && peek(1) == '\n') {
                pos_ += 2;
                while (pos_ < input_.size() && input_[pos_] == '\n') pos_++;
                return {TokenType::ParBreak, "\n\n", {start, pos_}};
            }

            if (c == '\n') {
                pos_++;
                return {TokenType::Newline, "\n", {start, pos_}};
            }

            if (c == ' ' || c == '\t') {
                std::string space;
                while (pos_ < input_.size() && (input_[pos_] == ' ' || input_[pos_] == '\t')) {
                    space += input_[pos_++];
                }
                return {TokenType::Text, space, {start, pos_}};
            }

            if (c == '%' && (pos_ == 0 || input_[pos_-1] != '\\'
                || (pos_ >= 2 && input_[pos_-2] == '\\'))) return lexComment();
            if (c == '%') {
                pos_++;
                return {TokenType::Text, "%", {start, pos_}};
            }

            if (c == '\\') return lexBackslash();
            if (c == '{') { pos_++; return {TokenType::OpenBrace, "{", {start, pos_}}; }
            if (c == '}') { pos_++; return {TokenType::CloseBrace, "}", {start, pos_}}; }
            if (c == '[') { pos_++; return {TokenType::OpenBracket, "[", {start, pos_}}; }
            if (c == ']') { pos_++; return {TokenType::CloseBracket, "]", {start, pos_}}; }

            return lexText();
        }

        Token lexComment() {
            size_t start = pos_;
            std::string text;
            text += consume();
            while (pos_ < input_.size() && input_[pos_] != '\n') {
                text += consume();
            }

            bool is_line_end = false;
            if (start > 0) {
                size_t i = start - 1;
                while (true) {
                    if (input_[i] == '\n') break;
                    if (input_[i] != ' ' && input_[i] != '\t') {
                        is_line_end = true;
                        break;
                    }
                    if (i == 0) break;
                    i--;
                }
            }
            return {TokenType::Comment, text, {start, pos_}, is_line_end};
        }

        Token lexBackslash() {
            size_t start = pos_;
            consume();

            if (pos_ >= input_.size()) return {TokenType::Text, "\\", {start, pos_}};

            char c = peek();

            if (c == '(') {
                pos_++;
                return {TokenType::InlineMathStart, "\\(", {start, pos_}};
            }
            if (c == ')') {
                pos_++;
                return {TokenType::InlineMathEnd, "\\)", {start, pos_}};
            }
            if (c == '[') {
                pos_++;
                return {TokenType::DisplayMathStart, "\\[", {start, pos_}};
            }
            if (c == ']') {
                pos_++;
                return {TokenType::DisplayMathEnd, "\\]", {start, pos_}};
            }

            if (!isalpha(c)) {
                std::string val = "\\";
                val += consume();
                return {TokenType::Text, val, {start, pos_}};
            }

            std::string cmd;
            while (pos_ < input_.size() && isalpha(input_[pos_])) {
                cmd += consume();
            }

            if (cmd == "begin" || cmd == "end") {
                size_t saved_pos = pos_;
                skipWhitespace();

                if (peek() == '{') {
                    consume();
                    std::string env_name;
                    while (pos_ < input_.size() && input_[pos_] != '}') {
                        env_name += consume();
                    }
                    if (peek() == '}') consume();

                    TokenType tt = (cmd == "begin") ? TokenType::BeginEnv : TokenType::EndEnv;
                    Token tok = {tt, env_name, {start, pos_}};

                    if (cmd == "begin") {
                        auto* rule = registry_.lookupEnv(env_name);
                        if (rule && rule->is_verbatim) {
                            mode_ = LexMode::VerbatimSeekEnd;
                            verbatim_end_tag_ = "\\end{" + env_name + "}";
                        }
                    }
                    return tok;
                } else {
                    pos_ = saved_pos;
                }
            } else if (cmd == "verb" || cmd == "verb*") {
                if (cmd == "verb" && peek() == '*') {
                    consume();
                    cmd += '*';
                }
                mode_ = LexMode::VerbSeekDelim;
                return {TokenType::Command, cmd, {start, pos_}};
            }

            return {TokenType::Command, cmd, {start, pos_}};
        }

        Token lexVerbSeekDelim() {
            size_t start = pos_;
            if (pos_ >= input_.size()) return {TokenType::Eof, "", {start, pos_}};

            char delim = consume();
            std::string content;
            content += delim;

            while (pos_ < input_.size() && input_[pos_] != delim) {
                content += consume();
            }
            if (pos_ < input_.size()) content += consume();

            mode_ = LexMode::Normal;
            return {TokenType::VerbContent, content, {start, pos_}};
        }

        Token lexVerbatimSeekEnd() {
            size_t start = pos_;
            std::string content;

            while (pos_ < input_.size()) {
                if (input_.compare(pos_, verbatim_end_tag_.size(), verbatim_end_tag_) == 0) {
                    break;
                }
                content += consume();
            }

            mode_ = LexMode::Normal;
            return {TokenType::VerbatimContent, content, {start, pos_}};
        }

        Token lexText() {
            size_t start = pos_;
            std::string text;
            while (pos_ < input_.size()) {
                char c = peek();
                if (c == '\\' || c == '%' || c == '{' || c == '}' || c == '[' || c == ']' || c == '\n' || c == ' ' || c == '\t' || c == '$') {
                    break;
                }
                text += consume();
            }
            if (text.empty() && pos_ < input_.size()) text += consume();
            return {TokenType::Text, text, {start, pos_}};
        }
    };

} // namespace latex_fmt