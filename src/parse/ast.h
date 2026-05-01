#pragma once
#include <string>
#include <vector>
#include <memory>
#include "core/source_location.h"

namespace latex_fmt {

    enum class ParseContext {
        Text, InlineMath, DisplayMath, MathEnv, TikZ, Verbatim
    };

    struct ASTNode {
        SourceRange source;
        ParseContext context = ParseContext::Text;
        bool is_malformed = false;
        virtual ~ASTNode() = default;
    };

    struct Document : ASTNode {
        std::vector<std::unique_ptr<ASTNode>> children;
    };

    struct Environment : ASTNode {
        std::string name;
        std::vector<std::unique_ptr<ASTNode>> args;
        std::vector<std::unique_ptr<ASTNode>> children;
    };

    struct Command : ASTNode {
        std::string name;
        std::vector<std::unique_ptr<ASTNode>> args;
    };

    struct Text : ASTNode {
        std::string content;
    };

    struct CommentNode : ASTNode {
        std::string text;
        bool is_line_end;
    };

    struct ParBreak : ASTNode {};

    struct Newline : ASTNode {};

    struct InlineMath : ASTNode {
        bool is_dollar_form = true;
        std::vector<std::unique_ptr<ASTNode>> children;
    };

    struct Group : ASTNode {
        std::string delim_open;
        std::string delim_close;
        std::vector<std::unique_ptr<ASTNode>> children;
    };

    struct DisplayMath : ASTNode {
        bool is_dollar_form = true;
        std::vector<std::unique_ptr<ASTNode>> children;
    };

} // namespace latex_fmt
