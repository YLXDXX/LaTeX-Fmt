#pragma once
#include <vector>
#include <string_view>
#include "lexer.h"
#include "ast.h"
#include "core/registry.h"

namespace latex_fmt {

    class Parser {
    public:
        Parser(std::vector<Token> tokens, std::string_view source, const Registry& registry)
        : tokens_(std::move(tokens)), source_(source), registry_(registry), pos_(0) {}

        std::unique_ptr<Document> parse() {
            auto doc = std::make_unique<Document>();
            doc->source = {0, source_.size()};
            while (peek().type != TokenType::Eof) {
                auto child = parseNode(ParseContext::Text);
                if (child) doc->children.push_back(std::move(child));
            }
            return doc;
        }

    private:
        std::vector<Token> tokens_;
        std::string_view source_;
        const Registry& registry_;
        size_t pos_;

        const Token& peek() const { return tokens_[pos_]; }
        const Token& advance() { return tokens_[pos_++]; }

        std::string extractSource(SourceRange range) const {
            return std::string(source_.substr(range.begin_offset, range.end_offset - range.begin_offset));
        }

        std::unique_ptr<ASTNode> parseNode(ParseContext ctx) {
            const Token& tok = peek();

            switch (tok.type) {
                case TokenType::BeginEnv: return parseEnvironment(ctx);
                case TokenType::Command:  return parseCommand(ctx);
                case TokenType::Comment:  return parseComment();
                case TokenType::ParBreak: return parseParBreak();
                case TokenType::Newline:  return parseNewline();
                case TokenType::InlineMathStart: return parseInlineMath(ctx);
                case TokenType::DisplayMathStart: return parseDisplayMath(ctx);
                case TokenType::Text:
                case TokenType::VerbContent:
                case TokenType::VerbatimContent: {
                    auto node = parseTextOrVerb();
                    node->context = ctx;
                    return node;
                }
                case TokenType::OpenBrace: {
                    auto node = parseGroup(TokenType::OpenBrace, TokenType::CloseBrace);
                    node->context = ctx;
                    return node;
                }
                case TokenType::OpenBracket: {
                    auto node = parseGroup(TokenType::OpenBracket, TokenType::CloseBracket);
                    node->context = ctx;
                    return node;
                }
                case TokenType::CloseBrace: {
                    SourceRange range = tok.source;
                    advance();
                    auto node = std::make_unique<Text>();
                    node->source = range;
                    node->content = "}";
                    node->is_malformed = true;
                    return node;
                }
                default:
                    advance();
                    return nullptr;
            }
        }

        std::unique_ptr<InlineMath> parseInlineMath(ParseContext outer_ctx) {
            auto math = std::make_unique<InlineMath>();
            math->source.begin_offset = peek().source.begin_offset;
            math->is_dollar_form = (peek().value == "$");
            advance();

            while (peek().type != TokenType::InlineMathEnd && peek().type != TokenType::Eof) {
                auto child = parseNode(ParseContext::InlineMath);
                if (child) math->children.push_back(std::move(child));
            }

            if (peek().type == TokenType::InlineMathEnd) {
                math->source.end_offset = peek().source.end_offset;
                advance();
            } else {
                math->is_malformed = true;
                math->source.end_offset = peek().source.begin_offset;
            }

            math->context = ParseContext::InlineMath;
            return math;
        }

        std::unique_ptr<DisplayMath> parseDisplayMath(ParseContext outer_ctx) {
            auto math = std::make_unique<DisplayMath>();
            math->source.begin_offset = peek().source.begin_offset;
            math->is_dollar_form = (peek().value == "$$" || peek().value == "$");
            advance();

            while (peek().type != TokenType::DisplayMathEnd && peek().type != TokenType::Eof) {
                auto child = parseNode(ParseContext::DisplayMath);
                if (child) math->children.push_back(std::move(child));
            }

            if (peek().type == TokenType::DisplayMathEnd) {
                math->source.end_offset = peek().source.end_offset;
                advance();
            } else {
                math->is_malformed = true;
                math->source.end_offset = peek().source.begin_offset;
            }

            math->context = ParseContext::DisplayMath;
            return math;
        }

        std::unique_ptr<Environment> parseEnvironment(ParseContext outer_ctx) {
            auto env = std::make_unique<Environment>();
            env->name = peek().value;
            env->source.begin_offset = peek().source.begin_offset;
            advance();

            while (peek().type == TokenType::OpenBrace || peek().type == TokenType::OpenBracket) {
                if (peek().type == TokenType::OpenBracket) {
                    env->args.push_back(parseGroup(TokenType::OpenBracket, TokenType::CloseBracket));
                } else {
                    env->args.push_back(parseGroup(TokenType::OpenBrace, TokenType::CloseBrace));
                }
            }

            auto* rule = registry_.lookupEnv(env->name);
            ParseContext inner_ctx = outer_ctx;
            if (rule && rule->is_math) inner_ctx = ParseContext::MathEnv;
            if (rule && rule->is_verbatim) inner_ctx = ParseContext::Verbatim;

            while (peek().type != TokenType::EndEnv && peek().type != TokenType::Eof) {
                if (peek().type == TokenType::VerbatimContent) {
                    auto txt = std::make_unique<Text>();
                    txt->source = peek().source;
                    txt->content = peek().value;
                    txt->context = inner_ctx;
                    env->children.push_back(std::move(txt));
                    advance();
                } else {
                    auto child = parseNode(inner_ctx);
                    if (child) env->children.push_back(std::move(child));
                }
            }

            if (peek().type == TokenType::EndEnv && peek().value == env->name) {
                env->source.end_offset = peek().source.end_offset;
                advance();
            } else {
                env->is_malformed = true;
                env->source.end_offset = peek().source.begin_offset;
            }

            env->context = inner_ctx;
            return env;
        }

        std::unique_ptr<Command> parseCommand(ParseContext ctx) {
            auto cmd = std::make_unique<Command>();
            cmd->name = peek().value;
            cmd->source.begin_offset = peek().source.begin_offset;
            cmd->context = ctx;
            advance();

            auto* sig = registry_.lookupCmd(cmd->name);
            if (!sig) {
                while (peek().type == TokenType::OpenBrace || peek().type == TokenType::OpenBracket) {
                    if (peek().type == TokenType::OpenBracket) {
                        cmd->args.push_back(parseGroup(TokenType::OpenBracket, TokenType::CloseBracket));
                    } else {
                        cmd->args.push_back(parseGroup(TokenType::OpenBrace, TokenType::CloseBrace));
                    }
                }
                cmd->source.end_offset = (pos_ > 0) ? tokens_[pos_-1].source.end_offset : cmd->source.begin_offset;
                return cmd;
            }

            for (int i = 0; i < sig->optional_args; ++i) {
                if (peek().type == TokenType::OpenBracket) {
                    cmd->args.push_back(parseGroup(TokenType::OpenBracket, TokenType::CloseBracket));
                }
            }

            for (int i = 0; i < sig->mandatory_args; ++i) {
                if (peek().type == TokenType::OpenBrace) {
                    cmd->args.push_back(parseGroup(TokenType::OpenBrace, TokenType::CloseBrace));
                } else if (sig->mandatory_braces && peek().type == TokenType::Text && !peek().value.empty() && !tokens_[pos_].value.empty()) {
                    auto arg_node = std::make_unique<Text>();
                    arg_node->source.begin_offset = tokens_[pos_].source.begin_offset;
                    std::string val = tokens_[pos_].value;
                    int consume_len = 0;
                    if (val.size() >= 2 && val[0] == '\\') {
                        consume_len = 2;
                    } else {
                        consume_len = 1;
                    }
                    arg_node->content = val.substr(0, consume_len);
                    arg_node->source.end_offset = arg_node->source.begin_offset + consume_len;
                    cmd->args.push_back(std::move(arg_node));

                    tokens_[pos_].value = val.substr(consume_len);
                    tokens_[pos_].source.begin_offset += consume_len;
                    if (tokens_[pos_].value.empty()) advance();
                } else {
                    break;
                }
            }

            cmd->source.end_offset = (pos_ > 0) ? tokens_[pos_-1].source.end_offset : cmd->source.begin_offset;
            return cmd;
        }

        std::unique_ptr<Group> parseGroup(TokenType open, TokenType close) {
            auto group = std::make_unique<Group>();
            group->source.begin_offset = peek().source.begin_offset;
            group->delim_open = (open == TokenType::OpenBrace) ? "{" : "[";
            group->delim_close = (close == TokenType::CloseBrace) ? "}" : "]";
            advance();

            int depth = 1;

            while (peek().type != TokenType::Eof && depth > 0) {
                if (peek().type == open) {
                    auto inner = parseGroup(open, close);
                    group->children.push_back(std::move(inner));
                    if (depth > 1) depth--;
                } else if (peek().type == close) {
                    depth--;
                    if (depth == 0) {
                        group->source.end_offset = peek().source.end_offset;
                        advance();
                        break;
                    }
                } else if (peek().type == TokenType::Command) {
                    auto cmd = parseCommand(ParseContext::Text);
                    group->children.push_back(std::move(cmd));
                } else if (peek().type == TokenType::Comment) {
                    group->children.push_back(parseComment());
                } else if (peek().type == TokenType::ParBreak) {
                    group->children.push_back(parseParBreak());
                } else if (peek().type == TokenType::Newline) {
                    group->children.push_back(parseNewline());
                } else if (peek().type == TokenType::InlineMathStart) {
                    group->children.push_back(parseInlineMath(ParseContext::Text));
                } else if (peek().type == TokenType::DisplayMathStart) {
                    group->children.push_back(parseDisplayMath(ParseContext::Text));
                } else {
                    auto txt = parseTextOrVerb();
                    group->children.push_back(std::move(txt));
                }
            }

            if (depth > 0) {
                group->is_malformed = true;
                group->source.end_offset = source_.size();
            }
            return group;
        }

        std::unique_ptr<CommentNode> parseComment() {
            auto node = std::make_unique<CommentNode>();
            node->text = peek().value;
            node->source = peek().source;
            node->is_line_end = peek().is_line_end;
            advance();
            return node;
        }

        std::unique_ptr<ParBreak> parseParBreak() {
            auto node = std::make_unique<ParBreak>();
            node->source = peek().source;
            advance();
            return node;
        }

        std::unique_ptr<Newline> parseNewline() {
            auto node = std::make_unique<Newline>();
            node->source = peek().source;
            advance();
            return node;
        }

        std::unique_ptr<Text> parseTextOrVerb() {
            auto node = std::make_unique<Text>();
            node->content = peek().value;
            node->source = peek().source;
            if (peek().type == TokenType::VerbContent) {
                node->is_verbatim = true;
            }
            advance();
            return node;
        }
    };

} // namespace latex_fmt