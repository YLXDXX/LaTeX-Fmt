#include <catch2/catch_test_macros.hpp>
#include "parse/lexer.h"
#include "core/registry.h"

using namespace latex_fmt;

// 辅助函数：提取所有非 EOF token
static std::vector<Token> tokenize(const std::string& input) {
    Registry reg;
    reg.registerBuiltin();
    Lexer lexer(input, reg);
    auto tokens = lexer.tokenize();
    // 移除末尾 EOF
    if (!tokens.empty() && tokens.back().type == TokenType::Eof) {
        tokens.pop_back();
    }
    return tokens;
}

TEST_CASE("Lexer: verb delimiter extraction", "[lexer]") {
    SECTION("Pipe delimiter") {
        auto tokens = tokenize(R"(\verb|hello world|)");
        REQUIRE(tokens.size() == 2);
        REQUIRE(tokens[0].type == TokenType::Command);
        REQUIRE(tokens[0].value == "verb");
        REQUIRE(tokens[1].type == TokenType::VerbContent);
        REQUIRE(tokens[1].value == "|hello world|");
    }

    SECTION("Bang delimiter") {
        auto tokens = tokenize(R"(\verb!special chars!)");
        REQUIRE(tokens.size() == 2);
        REQUIRE(tokens[0].type == TokenType::Command);
        REQUIRE(tokens[0].value == "verb");
        REQUIRE(tokens[1].type == TokenType::VerbContent);
        REQUIRE(tokens[1].value == "!special chars!");
    }

    SECTION("Plus delimiter") {
        auto tokens = tokenize(R"(\verb+plus sign+)");
        REQUIRE(tokens.size() == 2);
        REQUIRE(tokens[0].type == TokenType::Command);
        REQUIRE(tokens[0].value == "verb");
        REQUIRE(tokens[1].type == TokenType::VerbContent);
        REQUIRE(tokens[1].value == "+plus sign+");
    }

    SECTION("verb* variant") {
        auto tokens = tokenize(R"(\verb*|show spaces|)");
        REQUIRE(tokens.size() == 2);
        REQUIRE(tokens[0].type == TokenType::Command);
        REQUIRE(tokens[0].value == "verb*");
        REQUIRE(tokens[1].type == TokenType::VerbContent);
        REQUIRE(tokens[1].value == "|show spaces|");
    }
}

TEST_CASE("Lexer: verbatim environment", "[lexer]") {
    SECTION("Basic verbatim") {
        auto tokens = tokenize(R"(\begin{verbatim} \frac{1}{2} \end{verbatim})");
        REQUIRE(tokens.size() == 3);
        REQUIRE(tokens[0].type == TokenType::BeginEnv);
        REQUIRE(tokens[0].value == "verbatim");
        REQUIRE(tokens[1].type == TokenType::VerbatimContent);
        REQUIRE(tokens[1].value == " \\frac{1}{2} ");
        REQUIRE(tokens[2].type == TokenType::EndEnv);
        REQUIRE(tokens[2].value == "verbatim");
    }

    SECTION("Verbatim preserves comments") {
        auto tokens = tokenize("\\begin{verbatim}% not a comment\n\\end{verbatim}");
        REQUIRE(tokens.size() == 3);
        REQUIRE(tokens[1].type == TokenType::VerbatimContent);
        REQUIRE(tokens[1].value == "% not a comment\n");
    }

    SECTION("Verbatim preserves commands") {
        auto tokens = tokenize("\\begin{verbatim}\\section{Title}\\end{verbatim}");
        REQUIRE(tokens.size() == 3);
        REQUIRE(tokens[1].value == "\\section{Title}");
    }

    SECTION("lstlisting environment") {
        auto tokens = tokenize("\\begin{lstlisting}code\\end{lstlisting}");
        REQUIRE(tokens.size() == 3);
        REQUIRE(tokens[0].type == TokenType::BeginEnv);
        REQUIRE(tokens[0].value == "lstlisting");
        REQUIRE(tokens[1].type == TokenType::VerbatimContent);
        REQUIRE(tokens[2].type == TokenType::EndEnv);
        REQUIRE(tokens[2].value == "lstlisting");
    }

    SECTION("minted environment") {
        auto tokens = tokenize("\\begin{minted}python\\end{minted}");
        REQUIRE(tokens.size() == 3);
        REQUIRE(tokens[0].value == "minted");
        REQUIRE(tokens[1].type == TokenType::VerbatimContent);
    }
}

TEST_CASE("Lexer: comment tokenization", "[lexer]") {
    SECTION("Standalone comment") {
        auto tokens = tokenize("% standalone");
        REQUIRE(tokens.size() == 1);
        REQUIRE(tokens[0].type == TokenType::Comment);
        REQUIRE(tokens[0].value == "% standalone");
        REQUIRE(tokens[0].is_line_end == false);
    }

    SECTION("Line-end comment") {
        auto tokens = tokenize("code % comment");
        // Text("code"), Text(" "), Comment("% comment")
        REQUIRE(tokens.size() == 3);
        REQUIRE(tokens[2].type == TokenType::Comment);
        REQUIRE(tokens[2].value == "% comment");
        REQUIRE(tokens[2].is_line_end == true);
    }

    SECTION("Comment at start of line is standalone") {
        auto tokens = tokenize("% comment\n");
        REQUIRE(tokens[0].type == TokenType::Comment);
        REQUIRE(tokens[0].is_line_end == false);
    }

    SECTION("Empty comment (just percent)") {
        auto tokens = tokenize("%");
        REQUIRE(tokens.size() == 1);
        REQUIRE(tokens[0].type == TokenType::Comment);
        REQUIRE(tokens[0].value == "%");
    }

    SECTION("Comment with leading spaces") {
        // "   % indented comment" — the % is after spaces, but no non-space char before it
        auto tokens = tokenize("   % indented");
        // Space token, then comment
        REQUIRE(tokens.back().type == TokenType::Comment);
        REQUIRE(tokens.back().value == "% indented");
        REQUIRE(tokens.back().is_line_end == false);
    }

    SECTION("Multiple comments on consecutive lines") {
        auto tokens = tokenize("% line1\n% line2\n% line3");
        REQUIRE(tokens.size() == 5); // 3 comments + 2 newlines
        for (auto& t : tokens) {
            if (t.type == TokenType::Comment) {
                REQUIRE(t.is_line_end == false); // standalone comments
            }
        }
    }
}

TEST_CASE("Lexer: environment begin/end", "[lexer]") {
    SECTION("Basic begin") {
        auto tokens = tokenize("\\begin{equation}");
        REQUIRE(tokens.size() == 1);
        REQUIRE(tokens[0].type == TokenType::BeginEnv);
        REQUIRE(tokens[0].value == "equation");
    }

    SECTION("Basic end") {
        auto tokens = tokenize("\\end{equation}");
        REQUIRE(tokens.size() == 1);
        REQUIRE(tokens[0].type == TokenType::EndEnv);
        REQUIRE(tokens[0].value == "equation");
    }

    SECTION("Starred environment") {
        auto tokens = tokenize("\\begin{align*}");
        REQUIRE(tokens[0].type == TokenType::BeginEnv);
        REQUIRE(tokens[0].value == "align*");
    }

    SECTION("Environment with spaces before brace") {
        auto tokens = tokenize("\\begin{  equation  }");
        REQUIRE(tokens[0].type == TokenType::BeginEnv);
        REQUIRE(tokens[0].value == "  equation  ");
    }
}

TEST_CASE("Lexer: ParBreak and Newline", "[lexer]") {
    SECTION("Single newline") {
        auto tokens = tokenize("a\nb");
        // Text("a"), Newline, Text("b")
        REQUIRE(tokens.size() == 3);
        REQUIRE(tokens[1].type == TokenType::Newline);
    }

    SECTION("Double newline is ParBreak") {
        auto tokens = tokenize("a\n\nb");
        // Text("a"), ParBreak, Text("b")
        REQUIRE(tokens.size() == 3);
        REQUIRE(tokens[1].type == TokenType::ParBreak);
    }

    SECTION("Triple newline is still one ParBreak") {
        auto tokens = tokenize("a\n\n\nb");
        REQUIRE(tokens.size() == 3);
        REQUIRE(tokens[1].type == TokenType::ParBreak);
    }

    SECTION("Many blank lines compress to one ParBreak") {
        auto tokens = tokenize("a\n\n\n\n\nb");
        REQUIRE(tokens.size() == 3);
        REQUIRE(tokens[1].type == TokenType::ParBreak);
    }

    SECTION("ParBreak at start of input") {
        auto tokens = tokenize("\n\ntext");
        REQUIRE(tokens[0].type == TokenType::ParBreak);
        REQUIRE(tokens[1].type == TokenType::Text);
    }
}

TEST_CASE("Lexer: command tokenization", "[lexer]") {
    SECTION("Alpha commands") {
        auto tokens = tokenize("\\frac");
        REQUIRE(tokens[0].type == TokenType::Command);
        REQUIRE(tokens[0].value == "frac");
    }

    SECTION("begin is BeginEnv not Command") {
        auto tokens = tokenize("\\begin{equation}");
        REQUIRE(tokens[0].type == TokenType::BeginEnv);
    }

    SECTION("end is EndEnv not Command") {
        auto tokens = tokenize("\\end{equation}");
        REQUIRE(tokens[0].type == TokenType::EndEnv);
    }

    SECTION("Non-alpha commands become Text") {
        auto tokens = tokenize("\\\\");
        REQUIRE(tokens[0].type == TokenType::Text);
        REQUIRE(tokens[0].value == "\\\\");
    }

    SECTION("Backslash-brace") {
        auto tokens = tokenize("\\{");
        REQUIRE(tokens[0].type == TokenType::Text);
        REQUIRE(tokens[0].value == "\\{");
    }

    SECTION("Backslash at end of input") {
        auto tokens = tokenize("\\");
        REQUIRE(tokens[0].type == TokenType::Text);
        REQUIRE(tokens[0].value == "\\");
    }
}

TEST_CASE("Lexer: text tokenization", "[lexer]") {
    SECTION("Simple word") {
        auto tokens = tokenize("hello");
        REQUIRE(tokens.size() == 1);
        REQUIRE(tokens[0].type == TokenType::Text);
        REQUIRE(tokens[0].value == "hello");
    }

    SECTION("Stops at special characters") {
        auto tokens = tokenize("abc%comment");
        REQUIRE(tokens[0].type == TokenType::Text);
        REQUIRE(tokens[0].value == "abc");
        REQUIRE(tokens[1].type == TokenType::Comment);
    }

    SECTION("Stops at backslash") {
        auto tokens = tokenize("text\\command");
        REQUIRE(tokens[0].type == TokenType::Text);
        REQUIRE(tokens[0].value == "text");
        REQUIRE(tokens[1].type == TokenType::Command);
    }

    SECTION("Spaces are separate tokens") {
        auto tokens = tokenize("a  b");
        // Text("a"), Text("  "), Text("b")
        REQUIRE(tokens.size() == 3);
        REQUIRE(tokens[0].value == "a");
        REQUIRE(tokens[1].value == "  ");
        REQUIRE(tokens[1].type == TokenType::Text);
        REQUIRE(tokens[2].value == "b");
    }
}

TEST_CASE("Lexer: braces and brackets", "[lexer]") {
    SECTION("Open brace") {
        auto tokens = tokenize("{");
        REQUIRE(tokens[0].type == TokenType::OpenBrace);
        REQUIRE(tokens[0].value == "{");
    }

    SECTION("Close brace") {
        auto tokens = tokenize("}");
        REQUIRE(tokens[0].type == TokenType::CloseBrace);
        REQUIRE(tokens[0].value == "}");
    }

    SECTION("Open bracket") {
        auto tokens = tokenize("[");
        REQUIRE(tokens[0].type == TokenType::OpenBracket);
        REQUIRE(tokens[0].value == "[");
    }

    SECTION("Close bracket") {
        auto tokens = tokenize("]");
        REQUIRE(tokens[0].type == TokenType::CloseBracket);
        REQUIRE(tokens[0].value == "]");
    }

    SECTION("Mixed braces and text") {
        auto tokens = tokenize("{abc}");
        REQUIRE(tokens[0].type == TokenType::OpenBrace);
        REQUIRE(tokens[1].type == TokenType::Text);
        REQUIRE(tokens[1].value == "abc");
        REQUIRE(tokens[2].type == TokenType::CloseBrace);
    }
}

TEST_CASE("Lexer: escaped percent is not a comment", "[lexer]") {
    SECTION("\\% is Text, not Comment") {
        auto tokens = tokenize("\\%");
        REQUIRE(tokens.size() == 1);
        REQUIRE(tokens[0].type == TokenType::Text);
        REQUIRE(tokens[0].value == "\\%");
    }

    SECTION("\\% in context") {
        auto tokens = tokenize("50\\% off");
        // Text("50"), Text("\\%"), Text(" "), Text("off")
        bool found_escaped = false;
        for (auto& t : tokens) {
            if (t.value == "\\%") {
                found_escaped = true;
                REQUIRE(t.type == TokenType::Text);
            }
        }
        REQUIRE(found_escaped);
    }
}

TEST_CASE("Lexer: EOF token", "[lexer]") {
    SECTION("Empty input produces only EOF") {
        Registry reg;
        reg.registerBuiltin();
        Lexer lexer("", reg);
        auto tokens = lexer.tokenize();
        REQUIRE(tokens.size() == 1);
        REQUIRE(tokens[0].type == TokenType::Eof);
    }

    SECTION("EOF is always last") {
        Registry reg;
        reg.registerBuiltin();
        Lexer lexer("hello", reg);
        auto tokens = lexer.tokenize();
        REQUIRE(tokens.back().type == TokenType::Eof);
    }
}

TEST_CASE("Lexer: source ranges", "[lexer]") {
    SECTION("Command source range") {
        auto tokens = tokenize("\\frac");
        REQUIRE(tokens[0].source.begin_offset == 0);
        REQUIRE(tokens[0].source.end_offset == 5);
    }

    SECTION("Text source range") {
        auto tokens = tokenize("abc");
        REQUIRE(tokens[0].source.begin_offset == 0);
        REQUIRE(tokens[0].source.end_offset == 3);
    }

    SECTION("Comment source range") {
        auto tokens = tokenize("% test");
        REQUIRE(tokens[0].source.begin_offset == 0);
        REQUIRE(tokens[0].source.end_offset == 6);
    }

    SECTION("Mixed content source ranges") {
        auto tokens = tokenize("a\\frac{b}");
        // Text("a") at 0-1
        REQUIRE(tokens[0].source.begin_offset == 0);
        REQUIRE(tokens[0].source.end_offset == 1);
        // Command("frac") at 1-6
        REQUIRE(tokens[1].source.begin_offset == 1);
        REQUIRE(tokens[1].source.end_offset == 6);
    }
}

TEST_CASE("Lexer: complex input", "[lexer]") {
    SECTION("Mixed tokens in sequence") {
        auto tokens = tokenize("\\section{Title}\\label{sec:1}");
        REQUIRE(tokens[0].type == TokenType::Command);
        REQUIRE(tokens[0].value == "section");
        REQUIRE(tokens[1].type == TokenType::OpenBrace);
        REQUIRE(tokens[2].type == TokenType::Text);
        REQUIRE(tokens[2].value == "Title");
        REQUIRE(tokens[3].type == TokenType::CloseBrace);
        REQUIRE(tokens[4].type == TokenType::Command);
        REQUIRE(tokens[4].value == "label");
    }

    SECTION("Nested braces in command args") {
        auto tokens = tokenize("\\textbf{\\textit{nested}}");
        REQUIRE(tokens[0].type == TokenType::Command);
        REQUIRE(tokens[0].value == "textbf");
        REQUIRE(tokens[1].type == TokenType::OpenBrace);
        // Inside: \textit{nested}
        REQUIRE(tokens[2].type == TokenType::Command);
        REQUIRE(tokens[2].value == "textit");
        }
        }

        // ══════════════════════════════════════════════════════════════
        // 以下测试需要 Lexer 重构后启用（添加 InlineMathStart/End,
        // DisplayMathStart/End 等 token 类型）
        // ══════════════════════════════════════════════════════════════

        /*
         T E*ST_CASE("Lexer: inline math delimiters", "[lexer][math]") {
         SECTION("Single dollar inline math") {
         auto tokens = tokenize("$x + y$");
         REQUIRE(tokens[0].type == TokenType::InlineMathStart);
         REQUIRE(tokens[1].type == TokenType::Text);
         REQUIRE(tokens[1].value == "x + y");
         REQUIRE(tokens[2].type == TokenType::InlineMathEnd);
         }

         SECTION("Double dollar is display math, not two inline") {
         auto tokens = tokenize("$$E = mc^2$$");
         REQUIRE(tokens[0].type == TokenType::DisplayMathStart);
         REQUIRE(tokens[1].type == TokenType::Text);
         REQUIRE(tokens[2].type == TokenType::DisplayMathEnd);
         }

         SECTION("Multiple inline math on same line") {
         auto tokens = tokenize("$a$ and $b$");
         REQUIRE(tokens[0].type == TokenType::InlineMathStart);
         REQUIRE(tokens[2].type == TokenType::InlineMathEnd);
         REQUIRE(tokens[4].type == TokenType::InlineMathStart);
         REQUIRE(tokens[6].type == TokenType::InlineMathEnd);
         }

         SECTION("Math stops at verbatim") {
         // $ inside \verb should not start math mode
         auto tokens = tokenize("\\verb|$|");
         REQUIRE(tokens[0].type == TokenType::Command);
         REQUIRE(tokens[1].type == TokenType::VerbContent);
         }
         }
         */
