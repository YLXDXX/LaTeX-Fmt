#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include "core/registry.h"
#include "parse/lexer.h"
#include "parse/parser.h"
#include "format/visitor.h"
#include <fstream>
#include <sstream>

namespace latex_fmt {

    std::string format_code(const std::string& input) {
        Registry registry;
        registry.registerBuiltin();

        Lexer lexer(input, registry);
        auto tokens = lexer.tokenize();

        Parser parser(std::move(tokens), input, registry);
        auto ast = parser.parse();

        FormatVisitor visitor(registry, input);
        visitor.visit(*ast);

        return visitor.extractOutput();
    }

    std::string format_code_config(const std::string& input, const FormatConfig& config) {
        Registry registry;
        registry.registerBuiltin();

        Lexer lexer(input, registry);
        auto tokens = lexer.tokenize();

        Parser parser(std::move(tokens), input, registry);
        auto ast = parser.parse();

        FormatVisitor visitor(registry, input, config);
        visitor.visit(*ast);

        return visitor.extractOutput();
    }

    // ═══════════════════════════════════════════════════════════
    //  快照测试：逐字节比对格式化输出与预期文件
    // ═══════════════════════════════════════════════════════════

    TEST_CASE("Snapshot tests", "[formatter][snapshot]") {
        auto run_snapshot = [](const std::string& name) {
            std::string base_path = SNAPSHOT_DIR;
            std::ifstream in_file(base_path + name + ".tex");
            std::ifstream exp_file(base_path + name + ".expected.tex");

            REQUIRE(in_file.is_open());
            REQUIRE(exp_file.is_open());

            std::string input((std::istreambuf_iterator<char>(in_file)),
                              std::istreambuf_iterator<char>());
            std::string expected((std::istreambuf_iterator<char>(exp_file)),
                                 std::istreambuf_iterator<char>());

            std::string output = format_code(input);
            INFO("Snapshot test: " << name);
            INFO("Output size: " << output.size()
            << ", Expected size: " << expected.size());
            REQUIRE(output == expected);
        };

        SECTION("Simple mixed text")           { run_snapshot("simple"); }
        SECTION("Text and spacing")            { run_snapshot("text_spacing"); }
        SECTION("Inline math")                 { run_snapshot("inline_math"); }
        SECTION("Display math")                { run_snapshot("display_math"); }
        SECTION("Math alignment")              { run_snapshot("math_align"); }
        SECTION("Math misc")                   { run_snapshot("math_misc"); }
        SECTION("Commands")                    { run_snapshot("commands"); }
        SECTION("Comments")                    { run_snapshot("comments"); }
        SECTION("Environments")                { run_snapshot("environments"); }
        SECTION("Verbatim")                    { run_snapshot("verbatim"); }
        SECTION("Malformed recovery")          { run_snapshot("malformed"); }
        SECTION("Edge cases")                  { run_snapshot("edge_cases"); }
        SECTION("Comprehensive integration")   { run_snapshot("comprehensive"); }
    }

    // ═══════════════════════════════════════════════════════════
    //  幂等测试：格式化两次结果必须完全相同
    // ═══════════════════════════════════════════════════════════

    TEST_CASE("Idempotent: format input twice", "[formatter][idempotent]") {
        auto run_idempotent = [](const std::string& name) {
            std::string base_path = SNAPSHOT_DIR;
            std::ifstream in_file(base_path + name + ".tex");
            REQUIRE(in_file.is_open());

            std::string input((std::istreambuf_iterator<char>(in_file)),
                              std::istreambuf_iterator<char>());

            std::string first_fmt = format_code(input);
            std::string second_fmt = format_code(first_fmt);

            INFO("Idempotent test on input: " << name);
            REQUIRE(first_fmt == second_fmt);
        };

        SECTION("Simple")          { run_idempotent("simple"); }
        SECTION("Text spacing")    { run_idempotent("text_spacing"); }
        SECTION("Inline math")     { run_idempotent("inline_math"); }
        SECTION("Display math")    { run_idempotent("display_math"); }
        SECTION("Math alignment")  { run_idempotent("math_align"); }
        SECTION("Math misc")       { run_idempotent("math_misc"); }
        SECTION("Commands")        { run_idempotent("commands"); }
        SECTION("Comments")        { run_idempotent("comments"); }
        SECTION("Environments")    { run_idempotent("environments"); }
        SECTION("Verbatim")        { run_idempotent("verbatim"); }
        SECTION("Malformed")       { run_idempotent("malformed"); }
        SECTION("Edge cases")      { run_idempotent("edge_cases"); }
        SECTION("Comprehensive")   { run_idempotent("comprehensive"); }
    }

    // ═══════════════════════════════════════════════════════════
    //  预期文件幂等性：格式化 .expected.tex 必须输出自身
    //  这确保我们的"标准答案"本身就是稳定的
    // ═══════════════════════════════════════════════════════════

    TEST_CASE("Idempotent: expected files are stable", "[formatter][idempotent][expected]") {
        auto check_expected_stable = [](const std::string& name) {
            std::string base_path = SNAPSHOT_DIR;
            std::ifstream exp_file(base_path + name + ".expected.tex");
            REQUIRE(exp_file.is_open());

            std::string expected((std::istreambuf_iterator<char>(exp_file)),
                                 std::istreambuf_iterator<char>());

            std::string reformatted = format_code(expected);

            INFO("Expected file stability test: " << name);
            REQUIRE(reformatted == expected);
        };

        SECTION("Simple")          { check_expected_stable("simple"); }
        SECTION("Text spacing")    { check_expected_stable("text_spacing"); }
        SECTION("Inline math")     { check_expected_stable("inline_math"); }
        SECTION("Display math")    { check_expected_stable("display_math"); }
        SECTION("Math alignment")  { check_expected_stable("math_align"); }
        SECTION("Math misc")       { check_expected_stable("math_misc"); }
        SECTION("Commands")        { check_expected_stable("commands"); }
        SECTION("Comments")        { check_expected_stable("comments"); }
        SECTION("Environments")    { check_expected_stable("environments"); }
        SECTION("Verbatim")        { check_expected_stable("verbatim"); }
        SECTION("Malformed")       { check_expected_stable("malformed"); }
        SECTION("Edge cases")      { check_expected_stable("edge_cases"); }
        SECTION("Comprehensive")   { check_expected_stable("comprehensive"); }
    }

    // ═══════════════════════════════════════════════════════════
    //  R1: 数学定界符转换
    // ═══════════════════════════════════════════════════════════

    TEST_CASE("R1: delimiter conversion \\(\\) to $$", "[formatter][R1]") {
        SECTION("Simple inline math delimiter") {
            REQUIRE(format_code("\\(x + y\\)") == "$x + y$");
        }

        SECTION("Inline math with space") {
            REQUIRE(format_code("\\( x + y \\)") == "$ x + y $");
        }

        SECTION("Display math delimiter \\[\\] to $$$$") {
            REQUIRE(format_code("\\[E = mc^2\\]") == "$$\n  E = mc^2\n$$\n");
        }

        SECTION("Display math with space") {
            REQUIRE(format_code("\\[ E = mc^2 \\]") == "$$\n  E = mc^2\n$$\n");
        }
    }

    // ═══════════════════════════════════════════════════════════
    //  R2: 常见命令花括号补全
    // ═══════════════════════════════════════════════════════════

    TEST_CASE("R2: brace completion for \\frac", "[formatter][R2]") {
        SECTION("frac shorthand \\frac12") {
            REQUIRE(format_code("\\frac12") == "\\frac{1}{2}");
        }

        SECTION("frac with braces already") {
            REQUIRE(format_code("\\frac{1}{2}") == "\\frac{1}{2}");
        }

        SECTION("frac with longer content") {
            REQUIRE(format_code("\\frac{ab}{cd}") == "\\frac{ab}{cd}");
        }

        SECTION("frac34 in inline math") {
            REQUIRE(format_code("$\\frac34$") == "$\\frac{3}{4}$");
        }

        SECTION("frac with space between command and args") {
            REQUIRE(format_code("$\\frac 12 $") == "$\\frac{1}{2} $");
        }

        SECTION("frac with space between args") {
            REQUIRE(format_code("$\\frac1 2 $") == "$\\frac{1}{2} $");
        }

        SECTION("frac with spaces everywhere") {
            REQUIRE(format_code("$\\frac 1 2 $") == "$\\frac{1}{2} $");
        }

        SECTION("frac with space between args, no trailing space") {
            REQUIRE(format_code("$\\frac1 2$") == "$\\frac{1}{2}$");
        }

        SECTION("frac with space after command only") {
            REQUIRE(format_code("$\\frac 12$") == "$\\frac{1}{2}$");
        }
    }

    TEST_CASE("R2: brace completion for \\sqrt", "[formatter][R2]") {
        SECTION("sqrt with braces already") {
            REQUIRE(format_code("\\sqrt{2}") == "\\sqrt{2}");
        }

        SECTION("sqrt with optional arg and space") {
            REQUIRE(format_code("\\sqrt [3] 2") == "\\sqrt[3]{2}");
        }
    }

    TEST_CASE("R2: unknown commands are not modified", "[formatter][R2]") {
        SECTION("Unknown command with braces") {
            REQUIRE(format_code("\\mycommand{arg}") == "\\mycommand{arg}");
        }

        SECTION("Unknown command without braces") {
            REQUIRE(format_code("\\unknown") == "\\unknown");
        }

        SECTION("Unknown command with optional arg") {
            REQUIRE(format_code("\\custom[opt]{req}") == "\\custom[opt]{req}");
        }
    }

    // ═══════════════════════════════════════════════════════════
    //  R4: 空白处理
    // ═══════════════════════════════════════════════════════════

    TEST_CASE("R4: text mode space collapsing", "[formatter][R4]") {
        SECTION("Multiple spaces become one") {
            REQUIRE(format_code("hello   world") == "hello world");
        }

        SECTION("Tabs become one space") {
            REQUIRE(format_code("hello\tworld") == "hello world");
        }

        SECTION("Mixed tabs and spaces") {
            REQUIRE(format_code("hello \t world") == "hello world");
        }

        SECTION("No spaces stays no spaces") {
            REQUIRE(format_code("helloworld") == "helloworld");
        }

        SECTION("Single space stays single") {
            REQUIRE(format_code("hello world") == "hello world");
        }
    }

    TEST_CASE("R4: math mode preserves spaces", "[formatter][R4]") {
        SECTION("Inline math spaces not collapsed") {
            REQUIRE(format_code("$a  b$") == "$a  b$");
        }

        SECTION("Inline math spacing around equals") {
            REQUIRE(format_code("$x   =   y$") == "$x   =   y$");
        }
    }

    TEST_CASE("R4: trailing whitespace removed", "[formatter][R4]") {
        SECTION("Trailing spaces on line") {
            REQUIRE(format_code("text   ") == "text");
        }

        SECTION("Trailing tabs on line") {
            REQUIRE(format_code("text\t\t") == "text");
        }
    }

    TEST_CASE("R4: whitespace-only lines become empty", "[formatter][R4]") {
        SECTION("Line with only spaces") {
            REQUIRE(format_code("   \t  ") == "");
        }
    }

    TEST_CASE("R4: blank line compression", "[formatter][R4]") {
        SECTION("Multiple blank lines become one") {
            // 3 newlines = 2 blank lines → compress to 1 blank line
            REQUIRE(format_code("a\n\n\nb") == "a\n\nb");
        }

        SECTION("Many blank lines become one") {
            REQUIRE(format_code("a\n\n\n\n\nb") == "a\n\nb");
        }

        SECTION("Single blank line preserved") {
            REQUIRE(format_code("a\n\nb") == "a\n\nb");
        }
    }

    // ═══════════════════════════════════════════════════════════
    //  R5: 缩进
    // ═══════════════════════════════════════════════════════════

    TEST_CASE("R5: environment indentation", "[formatter][R5]") {
        SECTION("Basic environment") {
            REQUIRE(format_code("\\begin{itemize}\n\\item Hello\n\\end{itemize}") ==
            "\\begin{itemize}\n  \\item Hello\n\\end{itemize}");
        }

        SECTION("Nested environments") {
            REQUIRE(format_code(
                "\\begin{itemize}\n"
                "\\item A\n"
                "\\begin{itemize}\n"
                "\\item B\n"
                "\\end{itemize}\n"
                "\\end{itemize}"
            ) ==
            "\\begin{itemize}\n"
            "  \\item A\n"
            "  \\begin{itemize}\n"
            "    \\item B\n"
            "  \\end{itemize}\n"
            "\\end{itemize}");
        }

        SECTION("Equation environment") {
            REQUIRE(format_code("\\begin{equation}\nE = mc^2\n\\end{equation}") ==
            "\\begin{equation}\n  E = mc^2\n\\end{equation}");
        }

        SECTION("Document environment no extra indent") {
            REQUIRE(format_code("\\begin{document}\nHello\n\\end{document}") ==
            "\\begin{document}\nHello\n\\end{document}");
        }
    }

    // ═══════════════════════════════════════════════════════════
    //  注释处理
    // ═══════════════════════════════════════════════════════════

    TEST_CASE("Comment: line-end comment spacing", "[formatter][comment]") {
        SECTION("Add space before percent if missing") {
            REQUIRE(format_code("code%comment") == "code % comment");
        }

        SECTION("Add space after percent if missing") {
            REQUIRE(format_code("code %comment") == "code % comment");
        }

        SECTION("Normalize extra spaces in comment") {
            REQUIRE(format_code("code %  comment") == "code % comment");
        }

        SECTION("Already correct spacing preserved") {
            REQUIRE(format_code("code % comment") == "code % comment");
        }

        SECTION("Space before percent preserved, extra collapsed") {
            REQUIRE(format_code("code  %  comment") == "code % comment");
        }
    }

    TEST_CASE("Comment: standalone comment", "[formatter][comment]") {
        SECTION("Add space after percent") {
            REQUIRE(format_code("%comment") == "% comment");
        }

        SECTION("Normalize extra spaces") {
            REQUIRE(format_code("%  comment") == "% comment");
        }

        SECTION("Already correct") {
            REQUIRE(format_code("% comment") == "% comment");
        }

        SECTION("Empty comment stays empty") {
            REQUIRE(format_code("%") == "%");
        }
    }

    TEST_CASE("Comment: in math environment", "[formatter][comment]") {
        SECTION("Equation with line-end comment") {
            REQUIRE(format_code(
                "\\begin{equation}\nE = mc^2 % energy\n\\end{equation}"
            ) ==
            "\\begin{equation}\n  E = mc^2 % energy\n\\end{equation}");
        }
    }

    // ═══════════════════════════════════════════════════════════
    //  CJK-英文间距
    // ═══════════════════════════════════════════════════════════

    TEST_CASE("CJK-English spacing", "[formatter][cjk]") {
        SECTION("CJK followed by English needs space") {
            REQUIRE(format_code("中文English") == "中文 English");
        }

        SECTION("English followed by CJK needs space") {
            REQUIRE(format_code("English中文") == "English 中文");
        }

        SECTION("CJK punctuation before English: no space needed") {
            REQUIRE(format_code("中文，English") == "中文，English");
        }

        SECTION("English before CJK punctuation: no space needed") {
            REQUIRE(format_code("English，中文") == "English，中文");
        }

        SECTION("CJK before number needs space") {
            REQUIRE(format_code("第1个") == "第 1 个");
        }

        SECTION("Already spaced: no double space") {
            REQUIRE(format_code("中文 English") == "中文 English");
        }

        SECTION("Multiple transitions") {
            REQUIRE(format_code("中I文") == "中 I 文");
        }
    }

    TEST_CASE("CJK-math spacing", "[formatter][cjk]") {
        SECTION("CJK before inline math needs space") {
            REQUIRE(format_code("中文$x$") == "中文 $x$");
        }

        SECTION("Inline math before CJK needs space") {
            REQUIRE(format_code("$x$中文") == "$x$ 中文");
        }

        SECTION("CJK punctuation before math: no space") {
            REQUIRE(format_code("中文，$x$") == "中文，$x$");
        }

        SECTION("Math before CJK punctuation: no space") {
            REQUIRE(format_code("$x$，中文") == "$x$，中文");
        }

        SECTION("Already spaced: preserved") {
            REQUIRE(format_code("中文 $x$") == "中文 $x$");
        }
    }

    // ═══════════════════════════════════════════════════════════
    //  显示数学 $...$ 独立成行
    // ═══════════════════════════════════════════════════════════

    TEST_CASE("Display math: standalone lines", "[formatter][display_math]") {
        SECTION("Inline $$ wrapped to separate lines") {
            REQUIRE(format_code("text $$E=mc^2$$ more") ==
            "text\n$$\n  E=mc^2\n$$\nmore");
        }

        SECTION("$$ already on own line") {
            REQUIRE(format_code("$$\nE=mc^2\n$$") ==
            "$$\n  E=mc^2\n$$\n");
        }

        SECTION("CJK before display math") {
            REQUIRE(format_code("公式$$\\alpha$$这个") ==
            "公式\n$$\n  \\alpha\n$$\n这个");
        }
    }

    // ═══════════════════════════════════════════════════════════
    //  Verbatim 保留
    // ═══════════════════════════════════════════════════════════

    TEST_CASE("Verbatim: content preserved", "[formatter][verbatim]") {
        SECTION("Verbatim environment unchanged") {
            std::string input = "\\begin{verbatim}\n"
            "\\frac{1}{2} % not a comment\n"
            "\\end{verbatim}";
        REQUIRE(format_code(input) == input);
        }

        SECTION("Verb command preserved") {
            REQUIRE(format_code("\\verb|hello|") == "\\verb|hello|");
        }

        SECTION("Verb with bang delimiter") {
            REQUIRE(format_code("\\verb!hello!") == "\\verb!hello!");
        }
    }

    // ═══════════════════════════════════════════════════════════
    //  Malformed 恢复
    // ═══════════════════════════════════════════════════════════

    TEST_CASE("Malformed: unmatched close brace", "[formatter][malformed]") {
        SECTION("Unmatched } preserved") {
            std::string result = format_code("text } more");
            REQUIRE_THAT(result, Catch::Matchers::ContainsSubstring("}"));
        }
    }

    TEST_CASE("Malformed: missing end environment", "[formatter][malformed]") {
        SECTION("Missing \\end preserves content") {
            std::string result = format_code("\\begin{align}\nx &= 1\n");
            REQUIRE_THAT(result, Catch::Matchers::ContainsSubstring("x &= 1"));
        }
    }

    // ═══════════════════════════════════════════════════════════
    //  数学对齐
    // ═══════════════════════════════════════════════════════════

    TEST_CASE("Math alignment: align environment", "[formatter][align]") {
        SECTION("Single alignment point") {
            REQUIRE(format_code(
                "\\begin{align}\na &= b \\\\\nc &= d\n\\end{align}"
            ) ==
            "\\begin{align}\n  a & = b \\\\\n  c & = d\n\\end{align}");
        }

        SECTION("Multiple alignment points") {
            std::string result = format_code(
                "\\begin{align}\nx & = 1 & y & = 2 \\\\\nz & = 3 & w & = 4\n\\end{align}"
            );
            REQUIRE_THAT(result, Catch::Matchers::ContainsSubstring("\\begin{align}"));
            REQUIRE_THAT(result, Catch::Matchers::ContainsSubstring("\\end{align}"));
        }
    }

    TEST_CASE("Math alignment: matrix environment", "[formatter][align]") {
        SECTION("pmatrix alignment") {
            std::string result = format_code(
                "\\begin{equation}\n\\begin{pmatrix}\na & b \\\\\nc & d\n\\end{pmatrix}\n\\end{equation}"
            );
            REQUIRE_THAT(result, Catch::Matchers::ContainsSubstring("\\begin{pmatrix}"));
            REQUIRE_THAT(result, Catch::Matchers::ContainsSubstring("\\end{pmatrix}"));
        }
    }

    // ═══════════════════════════════════════════════════════════
    //  边界情况
    // ═══════════════════════════════════════════════════════════

    TEST_CASE("Edge: empty input", "[formatter][edge]") {
        REQUIRE(format_code("") == "");
    }

    TEST_CASE("Edge: only whitespace", "[formatter][edge]") {
        REQUIRE(format_code("   ") == "");
    }

    TEST_CASE("Edge: only newlines", "[formatter][edge]") {
        REQUIRE(format_code("\n\n\n") == "\n");
    }

    TEST_CASE("Edge: single character", "[formatter][edge]") {
        REQUIRE(format_code("a") == "a");
    }

    TEST_CASE("Edge: consecutive commands without spaces", "[formatter][edge]") {
        REQUIRE(format_code("\\textbf{A}\\textit{B}") == "\\textbf{A}\\textit{B}");
    }

    TEST_CASE("Edge: command after command", "[formatter][edge]") {
        REQUIRE(format_code("\\section{A}\\section{B}") == "\\section{A}\\section{B}");
    }

    TEST_CASE("Edge: escaped special characters", "[formatter][edge]") {
        SECTION("Escaped percent is not a comment") {
            std::string result = format_code("\\%");
            REQUIRE_THAT(result, Catch::Matchers::ContainsSubstring("\\%"));
            REQUIRE_THAT(result, !Catch::Matchers::ContainsSubstring("% "));
        }

        SECTION("Escaped braces") {
            REQUIRE(format_code("\\{ \\}") == "\\{ \\}");
        }

        SECTION("Escaped dollar") {
            REQUIRE(format_code("\\$") == "\\$");
        }

        SECTION("Escaped hash") {
            REQUIRE(format_code("\\#") == "\\#");
        }

        SECTION("Escaped ampersand") {
            REQUIRE(format_code("\\&") == "\\&");
        }

        SECTION("Escaped underscore") {
            REQUIRE(format_code("\\_") == "\\_");
        }
    }

    TEST_CASE("Edge: percent in math mode", "[formatter][edge]") {
        SECTION("\\% in inline math") {
            REQUIRE(format_code("$x \\% y$") == "$x \\% y$");
        }

        SECTION("\\% in equation") {
            std::string result = format_code("\\begin{equation}\n100 \\% = 1\n\\end{equation}");
            REQUIRE_THAT(result, Catch::Matchers::ContainsSubstring("100 \\% = 1"));
        }
    }

    TEST_CASE("Edge: tilde is non-breaking space", "[formatter][edge]") {
        SECTION("Tilde preserved in text") {
            REQUIRE(format_code("Table~\\ref{tab}") == "Table~\\ref{tab}");
        }
    }

    TEST_CASE("Edge: section with label", "[formatter][edge]") {
        REQUIRE(format_code("\\section{Title}\\label{sec:title}") ==
        "\\section{Title}\\label{sec:title}");
    }

    TEST_CASE("Edge: long line not wrapped", "[formatter][edge]") {
        std::string long_line = "This is a very long line of text that goes on and on and on.";
        REQUIRE(format_code(long_line) == long_line);
    }

    // ═══════════════════════════════════════════════════════════
    //  空组处理
    // ═══════════════════════════════════════════════════════════

    TEST_CASE("Edge: empty group", "[formatter][edge]") {
        SECTION("Empty braces in text") {
            REQUIRE(format_code("{}") == "{}");
        }

        SECTION("Empty braces in math") {
            REQUIRE(format_code("${}$") == "${}$");
        }
    }

    // ═══════════════════════════════════════════════════════════
    //  重音符命令处理
    // ═══════════════════════════════════════════════════════════

    TEST_CASE("Edge: LaTeX accent commands", "[formatter][edge]") {
        SECTION("Backslash-doublequote") {
            REQUIRE(format_code("\\\"{o}") == "\\\"{o}");
        }

        SECTION("Backslash-tilde") {
            REQUIRE(format_code("\\~{n}") == "\\~{n}");
        }

        SECTION("Backslash-caret") {
            REQUIRE(format_code("\\^{i}") == "\\^{i}");
        }

        SECTION("Backslash-backtick") {
            REQUIRE(format_code("\\`{a}") == "\\`{a}");
        }

        SECTION("Backslash-quote") {
            REQUIRE(format_code("\\'{e}") == "\\'{e}");
        }

        SECTION("Backslash-cedilla") {
            REQUIRE(format_code("\\c{c}") == "\\c{c}");
        }
    }

    // ═══════════════════════════════════════════════════════════
    //  \usepackage 不补全花括号测试
    // ═══════════════════════════════════════════════════════════

    TEST_CASE("Edge: usepackage brace non-completion", "[formatter][edge]") {
        SECTION("usepackage keeps its braces") {
            REQUIRE(format_code("\\usepackage{amsmath}") == "\\usepackage{amsmath}");
        }

        SECTION("usepackage with optional arg") {
            REQUIRE(format_code("\\usepackage[utf8]{inputenc}") == "\\usepackage[utf8]{inputenc}");
        }
    }

    // ═══════════════════════════════════════════════════════════
    //  Math spacing commands
    // ═══════════════════════════════════════════════════════════

    TEST_CASE("Edge: math spacing commands", "[formatter][edge][math]") {
        SECTION("thin space") {
            REQUIRE(format_code("$a\\,b$") == "$a\\,b$");
        }

        SECTION("thick space") {
            REQUIRE(format_code("$a\\;b$") == "$a\\;b$");
        }

        SECTION("quad") {
            REQUIRE(format_code("$a\\quad b$") == "$a\\quad b$");
        }

        SECTION("qquad") {
            REQUIRE(format_code("$a\\qquad b$") == "$a\\qquad b$");
        }

        SECTION("negative thin space") {
            REQUIRE(format_code("$a\\!b$") == "$a\\!b$");
        }
    }

    // ═══════════════════════════════════════════════════════════
    //  Nested \frac shorthand
    // ═══════════════════════════════════════════════════════════

    TEST_CASE("Edge: nested frac shorthand", "[formatter][edge][R2]") {
        SECTION("Double frac in one line") {
            REQUIRE(format_code("\\frac12\\frac34") == "\\frac{1}{2}\\frac{3}{4}");
        }

        SECTION("Frac inside frac") {
            std::string result = format_code("\\frac{\\frac12}{\\frac34}");
            REQUIRE_THAT(result, Catch::Matchers::ContainsSubstring("\\frac{"));
            REQUIRE(result == "\\frac{\\frac{1}{2}}{\\frac{3}{4}}");
        }
    }

    // ═══════════════════════════════════════════════════════════
    //  Tab character handling
    // ═══════════════════════════════════════════════════════════

    TEST_CASE("Edge: tab character normalization", "[formatter][edge]") {
        SECTION("Tab becomes space") {
            REQUIRE(format_code("a\tb") == "a b");
        }

        SECTION("Tab after CJK") {
            REQUIRE(format_code("中文\tEnglish") == "中文 English");
        }

        SECTION("Tab in indentation") {
            REQUIRE(format_code("  \ttext") == "text");
        }
    }

    // ═══════════════════════════════════════════════════════════
    //  Mixed edge scenarios
    // ═══════════════════════════════════════════════════════════

    TEST_CASE("Edge: non-breaking space tie", "[formatter][edge]") {
        SECTION("Tilde preserved with ref") {
            REQUIRE(format_code("Table~\\ref{tab}") == "Table~\\ref{tab}");
        }

        SECTION("Tilde with CJK") {
            REQUIRE(format_code("中文~English") == "中文~English");
        }
    }

    TEST_CASE("Edge: percent as non-comment character", "[formatter][edge]") {
        SECTION("Escaped percent at start") {
            REQUIRE(format_code("\\% text") == "\\% text");
        }

        SECTION("Escaped percent at end") {
            REQUIRE(format_code("text \\%") == "text \\%");
        }
    }

    // ═══════════════════════════════════════════════════════════
    //  Config=false 路径测试
    // ═══════════════════════════════════════════════════════════

    TEST_CASE("Config: display_math_format=false", "[formatter][config]") {
        FormatConfig cfg;
        cfg.display_math_format = false;

        SECTION("Inline display math preserved") {
            REQUIRE(format_code_config("$$x+y$$", cfg) == "$$x+y$$");
        }

        SECTION("Display math with CJK before preserved") {
            auto result = format_code_config("你好$$x+y$$世界", cfg);
            REQUIRE(result.find("$$x+y$$") != std::string::npos);
        }
    }

    TEST_CASE("Config: math_delimiter_unify=false", "[formatter][config]") {
        FormatConfig cfg;
        cfg.math_delimiter_unify = false;

        SECTION("Inline LaTeX delimiters preserved") {
            REQUIRE(format_code_config("\\(x+y\\)", cfg) == "\\(x+y\\)");
        }

        SECTION("Display LaTeX delimiters preserved in output") {
            auto result = format_code_config("\\[x+y\\]", cfg);
            REQUIRE(result.find("\\[") != std::string::npos);
            REQUIRE(result.find("\\]") != std::string::npos);
        }
    }

    TEST_CASE("Config: cjk_spacing=false", "[formatter][config]") {
        FormatConfig cfg;
        cfg.cjk_spacing = false;

        SECTION("No space inserted") {
            REQUIRE(format_code_config("你好World", cfg) == "你好World");
        }

        SECTION("Math still has CJK spacing") {
            auto result = format_code_config("你好$x$World", cfg);
            REQUIRE(result.find("你好") != std::string::npos);
            REQUIRE(result.find("World") != std::string::npos);
        }
    }

    TEST_CASE("Config: brace_completion=false", "[formatter][config]") {
        FormatConfig cfg;
        cfg.brace_completion = false;

        SECTION("Frac shorthand preserved") {
            REQUIRE(format_code_config("\\frac12", cfg) == "\\frac12");
        }

        SECTION("Sqrt shorthand preserved") {
            REQUIRE(format_code_config("\\sqrt2", cfg) == "\\sqrt2");
        }
    }

    TEST_CASE("Config: comment_normalize=false", "[formatter][config]") {
        FormatConfig cfg;
        cfg.comment_normalize = false;

        SECTION("Raw comment preserved") {
            REQUIRE(format_code_config("%rawcomment", cfg) == "%rawcomment");
        }

        SECTION("Empty comment unchanged") {
            REQUIRE(format_code_config("%", cfg) == "%");
        }
    }

    // ═══════════════════════════════════════════════════════════
    //  Malformed / error recovery 边界测试
    // ═══════════════════════════════════════════════════════════

    TEST_CASE("Malformed: deeply nested unmatched braces", "[formatter][malformed]") {
        SECTION("Three unmatched opens") {
            auto result = format_code("{{{x}");
            REQUIRE_FALSE(result.empty());
        }

        SECTION("Unmatched close at start") {
            auto result = format_code("}text");
            REQUIRE_FALSE(result.empty());
        }
    }

    TEST_CASE("Malformed: mixed malformed scenarios", "[formatter][malformed]") {
        SECTION("Command with malformed args") {
            auto result = format_code("\\frac{1");
            REQUIRE_FALSE(result.empty());
        }

        SECTION("Nested begin without end") {
            auto result = format_code("\\begin{document}\\begin{figure}text");
            REQUIRE_FALSE(result.empty());
        }
    }

    // ═══════════════════════════════════════════════════════════
    //  真实场景 / AI 生成 / 粘贴内容测试
    // ═══════════════════════════════════════════════════════════

    TEST_CASE("Real-world: AI-generated LaTeX patterns", "[formatter][realworld]") {
        SECTION("text in math with CJK") {
            auto result = format_code("$f(x) = \\text{当}x > 0\\text{时}$");
            REQUIRE_FALSE(result.empty());
        }

        SECTION("Common ChatGPT LaTeX output") {
            auto input = "\\documentclass{article}\n\\begin{document}\n"
                         "The formula is $E = mc^2$ where $c$ is the speed of light.\n"
                         "\\begin{align}\nf(x) &= x^2 + 2x + 1 \\\\\n&= (x+1)^2\n\\end{align}\n"
                         "\\end{document}\n";
            auto result = format_code(input);
            REQUIRE(result.find("\\frac") == std::string::npos);
            REQUIRE(result.find("align") != std::string::npos);
        }
    }

    TEST_CASE("Real-world: pasted web content", "[formatter][realworld]") {
        SECTION("HTML entities in text") {
            auto result = format_code("x &gt; 0 and y &lt; 1");
            REQUIRE_FALSE(result.empty());
        }

        SECTION("Unicode smart quotes") {
            auto result = format_code("\"\u201Cquoted text\u201D he said");
            REQUIRE_FALSE(result.empty());
        }

        SECTION("Mixed tabs and spaces from paste") {
            auto result = format_code(" \t  hello  \t\t world\t \n");
            REQUIRE_FALSE(result.empty());
        }
    }

    TEST_CASE("Real-world: common LaTeX constructs", "[formatter][realworld]") {
        SECTION("Item with optional argument") {
            REQUIRE(format_code("\\item[Step 1] Do something") == "\\item[Step 1] Do something");
        }

        SECTION("Figure with position and label") {
            auto result = format_code(
                "\\begin{figure}[h]\n"
                "\\includegraphics{img.png}\n"
                "\\caption{Test}\n"
                "\\label{fig:test}\n"
                "\\end{figure}\n");
            REQUIRE(result.find("\\begin{figure}[h]") != std::string::npos);
            REQUIRE(result.find("\\end{figure}") != std::string::npos);
        }

        SECTION("Deeply nested itemize") {
            auto result = format_code(
                "\\begin{itemize}\n"
                "\\item A\n"
                "\\begin{itemize}\n"
                "\\item B\n"
                "\\begin{itemize}\n"
                "\\item C\n"
                "\\end{itemize}\n"
                "\\end{itemize}\n"
                "\\end{itemize}\n");
            REQUIRE(result.find("\\item C") != std::string::npos);
        }

        SECTION("newcommand definition") {
            REQUIRE(format_code("\\newcommand{\\foo}[2]{#1 \\to #2}") ==
                    "\\newcommand{\\foo}[2]{#1 \\to #2}");
        }
    }

    TEST_CASE("Real-world: CJK edge cases", "[formatter][realworld]") {
        SECTION("CJK fullwidth comma at boundary") {
            auto result = format_code("你好,world");
            REQUIRE_FALSE(result.empty());
        }

        SECTION("Mixed CJK and Latin inline math") {
            auto result = format_code("令$x\\in\\mathbb{R}$满足条件");
            REQUIRE_FALSE(result.empty());
        }
    }

} // namespace latex_fmt
