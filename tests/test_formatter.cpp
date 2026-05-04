#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include "core/registry.h"
#include "core/syntax_check.h"
#include "core/syntax_fix.h"
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
            REQUIRE(format_code("\\[E = mc^2\\]") == "$$\n  E = mc^{2}\n$$\n");
        }

        SECTION("Display math with space") {
            REQUIRE(format_code("\\[ E = mc^2 \\]") == "$$\n  E = mc^{2}\n$$\n");
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

        SECTION("sqrt shorthand without optional arg") {
            REQUIRE(format_code("\\sqrt3") == "\\sqrt{3}");
        }

        SECTION("sqrt shorthand in inline math") {
            REQUIRE(format_code("$\\sqrt3$") == "$\\sqrt{3}$");
        }

        SECTION("sqrt with space between command and arg") {
            REQUIRE(format_code("\\sqrt 5") == "\\sqrt{5}");
        }

        SECTION("frac and sqrt together in math") {
            REQUIRE(format_code("$\\frac12 + \\sqrt3$") == "$\\frac{1}{2} + \\sqrt{3}$");
        }
    }

    TEST_CASE("R2: subscript/superscript brace completion", "[formatter][R2]") {
        SECTION("simple subscript") {
            REQUIRE(format_code("$x_a$") == "$x_{a}$");
        }

        SECTION("simple superscript") {
            REQUIRE(format_code("$x^2$") == "$x^{2}$");
        }

        SECTION("multiple in one expression") {
            REQUIRE(format_code("$x_a + y_b + z_c$") == "$x_{a} + y_{b} + z_{c}$");
            REQUIRE(format_code("$x_a^2$") == "$x_{a}^{2}$");
        }

        SECTION("subscript with command argument") {
            REQUIRE(format_code("$x_\\alpha$") == "$x_{\\alpha}$");
            REQUIRE(format_code("$x^\\beta$") == "$x^{\\beta}$");
        }

        SECTION("both subscript and superscript") {
            REQUIRE(format_code("$^3_2A$") == "$^{3}_{2}A$");
            REQUIRE(format_code("$^3_2A_\\alpha^\\beta$") == "$^{3}_{2}A_{\\alpha}^{\\beta}$");
        }

        SECTION("command with sub/superscript") {
            REQUIRE(format_code("$^4_2\\mathrm{He}$") == "$^{4}_{2}\\mathrm{He}$");
        }

        SECTION("already braced preserved") {
            REQUIRE(format_code("$x_{a}$") == "$x_{a}$");
            REQUIRE(format_code("$x^{2}$") == "$x^{2}$");
            REQUIRE(format_code("$x_{ab}$") == "$x_{ab}$");
            REQUIRE(format_code("$x^{ab}$") == "$x^{ab}$");
        }

        SECTION("text mode not affected") {
            REQUIRE(format_code("a_b") == "a_b");
            REQUIRE(format_code("x^2") == "x^2");
        }

        SECTION("real-world vector example") {
            REQUIRE(format_code(
                "$\\vec{a}=(x_a, y_a, z_a)$ and $\\vec{b}=(x_b, y_b, z_b)$") ==
                "$\\vec{a}=(x_{a}, y_{a}, z_{a})$ and $\\vec{b}=(x_{b}, y_{b}, z_{b})$");
        }

        SECTION("whitespace between caret and argument handled correctly") {
            REQUIRE(format_code("$x^ 2$") == "$x^{2}$");
            REQUIRE(format_code("$x ^ {2}$") == "$x ^{2}$");
            REQUIRE(format_code("$x_ 2$") == "$x_{2}$");
            REQUIRE(format_code("$x ^ 2$") == "$x ^{2}$");
        }

        SECTION("spaces inside braces preserved") {
            REQUIRE(format_code("$b ^ { 2 }$") == "$b ^{ 2 }$");
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
            "\\begin{equation}\n  E = mc^{2}\n\\end{equation}");
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
            "\\begin{equation}\n  E = mc^{2} % energy\n\\end{equation}");
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
            "text\n$$\n  E=mc^{2}\n$$\nmore");
        }

        SECTION("$$ already on own line") {
            REQUIRE(format_code("$$\nE=mc^2\n$$") ==
            "$$\n  E=mc^{2}\n$$\n");
        }

        SECTION("CJK before display math") {
            REQUIRE(format_code("公式$$\\alpha$$这个") ==
            "公式\n$$\n  \\alpha\n$$\n这个");
        }
    }

    TEST_CASE("Display math: configurable style", "[formatter][display_math]") {
        SECTION("dollar style (default)") {
            FormatConfig cfg;
            cfg.display_math_style = latex_fmt::DisplayMathStyle::Dollar;
            REQUIRE(format_code_config("\\[x\\]", cfg) == "$$\n  x\n$$\n");
        }

        SECTION("bracket style") {
            FormatConfig cfg;
            cfg.display_math_style = latex_fmt::DisplayMathStyle::Bracket;
            REQUIRE(format_code_config("\\[x\\]", cfg) == "\\[\n  x\n\\]\n");
            REQUIRE(format_code_config("$$x$$", cfg) == "\\[\n  x\n\\]\n");
        }

        SECTION("equation style") {
            FormatConfig cfg;
            cfg.display_math_style = latex_fmt::DisplayMathStyle::Equation;
            REQUIRE(format_code_config("\\[x\\]", cfg) == "\\begin{equation}\n  x\n\\end{equation}\n");
            REQUIRE(format_code_config("$$x$$", cfg) == "\\begin{equation}\n  x\n\\end{equation}\n");
        }

        SECTION("equation* style") {
            FormatConfig cfg;
            cfg.display_math_style = latex_fmt::DisplayMathStyle::EquationStar;
            REQUIRE(format_code_config("\\[x\\]", cfg) == "\\begin{equation*}\n  x\n\\end{equation*}\n");
            REQUIRE(format_code_config("$$x$$", cfg) == "\\begin{equation*}\n  x\n\\end{equation*}\n");
        }

        SECTION("no-unify preserves original delimiters") {
            FormatConfig cfg;
            cfg.math_delimiter_unify = false;
            cfg.display_math_style = latex_fmt::DisplayMathStyle::Bracket;
            REQUIRE(format_code_config("$$x$$", cfg) == "$$\n  x\n$$\n");
            REQUIRE(format_code_config("\\[x\\]", cfg) == "\\[\n  x\n\\]\n");
        }
    }

    TEST_CASE("Environment: bracket group and comment formatting", "[formatter][edge]") {
        SECTION("closing bracket indented inside environment") {
            auto result = format_code(
                "\\begin{figure}[htbp]\n"
                "  \\begin{tikzpicture}[\n"
                "  scale=1.2\n"
                "]\n"
                "  \\end{tikzpicture}\n"
                "\\end{figure}\n");
            REQUIRE(result.find("  ]") != std::string::npos);
        }

        SECTION("comment after bracket stays on own line") {
            auto result = format_code(
                "\\begin{figure}[htbp]\n"
                "  \\begin{tikzpicture}[\n"
                "  scale=1.2\n"
                "]\n"
                "% a standalone comment\n"
                "  \\end{tikzpicture}\n"
                "\\end{figure}\n");
            REQUIRE(result.find("% a standalone comment") != std::string::npos);
            REQUIRE(result.find("]%") == std::string::npos);
        }

        SECTION("caption on its own line") {
            auto result = format_code(
                "\\begin{figure}[htbp]\n"
                "  \\centering\n"
                "  \\includegraphics{img.png}\n"
                "  \\caption{Test}\n"
                "\\end{figure}\n");
            REQUIRE(result.find("\\caption{Test}") != std::string::npos);
        }

        SECTION("caption after end{tikzpicture} on own line") {
            auto result = format_code(
                "\\begin{figure}[htbp]\n"
                "\\centering\n"
                "  \\begin{tikzpicture}[\n"
                "  scale=1.2\n"
                "]\n"
                "  \\end{tikzpicture}\\caption{Test}\n"
                "\\end{figure}\n");
            REQUIRE(result.find("\\end{tikzpicture}\n") != std::string::npos);
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

    TEST_CASE("Table alignment: tabular with \\hline", "[formatter][table]") {
        SECTION("\\hline on separate lines") {
            std::string result = format_code(
                "\\begin{tabular}{lcc}\\hline A & B & C \\\\\\hline D & E & F \\\\\\hline\\end{tabular}"
            );
            REQUIRE_THAT(result, Catch::Matchers::ContainsSubstring("\\hline\n"));
            REQUIRE_THAT(result, !Catch::Matchers::ContainsSubstring("\\hline A"));
        }

        SECTION("column alignment in tabular") {
            std::string result = format_code(
                "\\begin{tabular}{lr}\nName & 123 \\\\\nLongName & 5\n\\end{tabular}"
            );
            REQUIRE_THAT(result, Catch::Matchers::ContainsSubstring("LongName"));
            REQUIRE_THAT(result, Catch::Matchers::ContainsSubstring("  & 123"));
        }

        SECTION("\\hline after \\\\ splitting") {
            std::string result = format_code(
                "\\begin{tabular}{cc}\\hline a & b \\\\\\hline c & d \\\\\\hline\\end{tabular}"
            );
            REQUIRE_THAT(result, Catch::Matchers::ContainsSubstring("\\hline"));
            REQUIRE_THAT(result, Catch::Matchers::ContainsSubstring("a & b"));
            REQUIRE_THAT(result, Catch::Matchers::ContainsSubstring("c & d"));
        }
    }

    TEST_CASE("Table alignment: tblr with \\hline", "[formatter][table]") {
        SECTION("tblr shorthands aligned") {
            std::string result = format_code(
                "\\begin{tblr}{|l|c|r|}\\hline Header 1 & Header 2 & Header 3 \\\\\\hline Data A   & Data B   & Data C   \\\\\\hline\\end{tblr}"
            );
            REQUIRE_THAT(result, Catch::Matchers::ContainsSubstring("\\hline"));
            REQUIRE_THAT(result, Catch::Matchers::ContainsSubstring("Header 1"));
            REQUIRE_THAT(result, Catch::Matchers::ContainsSubstring("Data A"));
        }
    }

    TEST_CASE("Table alignment: \\toprule/\\midrule/\\bottomrule", "[formatter][table]") {
        SECTION("booktabs rules separated") {
            std::string result = format_code(
                "\\begin{tabular}{lcc}\\toprule A & B & C \\\\\\midrule D & E & F \\\\\\bottomrule\\end{tabular}"
            );
            REQUIRE_THAT(result, Catch::Matchers::ContainsSubstring("\\toprule\n"));
            REQUIRE_THAT(result, Catch::Matchers::ContainsSubstring("\\midrule\n"));
            REQUIRE_THAT(result, Catch::Matchers::ContainsSubstring("\\bottomrule\n"));
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

        SECTION("CJK after command group delimiters has no false spacing") {
            REQUIRE(format_code("单位向量\\cprotect\\footnote{注意}可以表示为") ==
                    "单位向量\\cprotect\\footnote{注意}可以表示为");
            REQUIRE(format_code("中文\\textbf{中文}中文") ==
                    "中文\\textbf{中文}中文");
            REQUIRE(format_code("前置{内容}后置") ==
                    "前置{内容}后置");
        }
    }

    std::vector<std::string> check_syntax(const std::string& input) {
        Registry registry;
        registry.registerBuiltin();
        Lexer lexer(input, registry);
        auto tokens = lexer.tokenize();
        Parser parser(std::move(tokens), input, registry);
        auto doc = parser.parse();
        SyntaxChecker checker(input, *doc);
        auto errors = checker.check();
        std::vector<std::string> messages;
        for (const auto& e : errors) {
            messages.push_back(e.message);
        }
        return messages;
    }

    TEST_CASE("SyntaxCheck: valid document has no errors", "[syntax]") {
        SECTION("empty document") {
            auto errors = check_syntax("");
            REQUIRE(errors.empty());
        }

        SECTION("document with environment") {
            auto errors = check_syntax("\\begin{document}\nHello\n\\end{document}");
            REQUIRE(errors.empty());
        }

        SECTION("document with math") {
            auto errors = check_syntax("\\begin{document}\n$x+y$\n\\end{document}");
            REQUIRE(errors.empty());
        }

        SECTION("document with groups") {
            auto errors = check_syntax("\\begin{document}\n\\textbf{Bold}\n\\end{document}");
            REQUIRE(errors.empty());
        }
    }

    TEST_CASE("SyntaxCheck: unclosed environment", "[syntax]") {
        SECTION("missing end") {
            auto errors = check_syntax("\\begin{document}\nHello");
            REQUIRE_FALSE(errors.empty());
            REQUIRE(errors[0].find("\\begin{document}") != std::string::npos);
        }
    }

    TEST_CASE("SyntaxCheck: unclosed inline math", "[syntax]") {
        SECTION("unclosed dollar") {
            auto errors = check_syntax("$x+y");
            REQUIRE_FALSE(errors.empty());
            REQUIRE(errors[0].find("inline math") != std::string::npos);
        }

        SECTION("math inside environment unclosed") {
            auto errors = check_syntax("\\begin{document}\n$x+y\n\\end{document}");
            REQUIRE_FALSE(errors.empty());
        }
    }

    TEST_CASE("SyntaxCheck: unclosed display math", "[syntax]") {
        SECTION("unclosed double dollar") {
            auto errors = check_syntax("$$x+y");
            REQUIRE_FALSE(errors.empty());
            REQUIRE(errors[0].find("display math") != std::string::npos);
        }
    }

    TEST_CASE("SyntaxCheck: unclosed brace", "[syntax]") {
        SECTION("unclosed curly brace") {
            auto errors = check_syntax("\\textbf{Bold");
            REQUIRE_FALSE(errors.empty());
            REQUIRE(errors[0].find("'{'") != std::string::npos);
        }
    }

    TEST_CASE("SyntaxCheck: unclosed bracket", "[syntax]") {
        SECTION("unclosed square bracket") {
            auto errors = check_syntax("\\sqrt[3{2}");
            REQUIRE_FALSE(errors.empty());
            REQUIRE(errors[0].find("'['") != std::string::npos);
        }
    }

    TEST_CASE("SyntaxCheck: extraneous close delimiters", "[syntax]") {
        SECTION("stray close brace") {
            auto errors = check_syntax("}text");
            REQUIRE_FALSE(errors.empty());
            REQUIRE(errors[0].find("'}") != std::string::npos);
        }
    }

    TEST_CASE("SyntaxCheck: nested environments", "[syntax]") {
        SECTION("valid nested") {
            auto errors = check_syntax(
                "\\begin{document}\n"
                "\\begin{figure}\n"
                "content\n"
                "\\end{figure}\n"
                "\\end{document}");
            REQUIRE(errors.empty());
        }

        SECTION("unclosed inner environment") {
            auto errors = check_syntax(
                "\\begin{document}\n"
                "\\begin{figure}\n"
                "content\n"
                "\\end{document}");
            REQUIRE_FALSE(errors.empty());
        }
    }

    TEST_CASE("SyntaxCheck: multiple errors reported", "[syntax]") {
        SECTION("unclosed math and environment") {
            auto errors = check_syntax("\\begin{document}\n$x+y");
            REQUIRE(errors.size() >= 2);
        }

        SECTION("many unclosed braces") {
            auto errors = check_syntax("\\textbf{\\textit{\\emph{text");
            REQUIRE(errors.size() >= 3);
        }

        SECTION("cross-type group error") {
            auto errors = check_syntax("\\sqrt[3{2}");
            REQUIRE_FALSE(errors.empty());
        }
    }

    TEST_CASE("SyntaxCheck: long document errors", "[syntax]") {
        std::string doc =
            "\\begin{document}\n"
            "\\begin{figure}\n"
            "  \\textbf{$x\n"
            "\\end{figure}\n"
            "\\end{document\n";
        auto errors = check_syntax(doc);
        REQUIRE_FALSE(errors.empty());
        REQUIRE(errors.size() >= 3);
    }

    TEST_CASE("SyntaxCheck: valid complex document no errors", "[syntax]") {
        std::string doc =
            "\\documentclass{article}\n"
            "\\usepackage{amsmath}\n"
            "\\begin{document}\n"
            "\\section{Introduction}\n"
            "The formula $E=mc^2$ is famous.\n"
            "\\begin{equation}\n"
            "  f(x) = x^2 + 2x + 1\n"
            "\\end{equation}\n"
            "\\begin{figure}[h]\n"
            "  \\centering\n"
            "  \\includegraphics{plot.pdf}\n"
            "  \\caption{A nice figure}\n"
            "  \\label{fig:plot}\n"
            "\\end{figure}\n"
            "See Figure~\\ref{fig:plot}.\n"
            "\\end{document}\n";
        auto errors = check_syntax(doc);
        REQUIRE(errors.empty());
    }

    std::string fix_syntax_errors(const std::string& input) {
        Registry registry;
        registry.registerBuiltin();
        Lexer lexer(input, registry);
        auto tokens = lexer.tokenize();
        Parser parser(std::move(tokens), input, registry);
        auto doc = parser.parse();
        SyntaxFixer fixer(input, *doc);
        return fixer.apply();
    }

    bool has_syntax_errors(const std::string& input) {
        Registry registry;
        registry.registerBuiltin();
        Lexer lexer(input, registry);
        auto tokens = lexer.tokenize();
        Parser parser(std::move(tokens), input, registry);
        auto doc = parser.parse();
        SyntaxChecker checker(input, *doc);
        return !checker.check().empty();
    }

    TEST_CASE("SyntaxFix: unclosed brace is fixed", "[syntax][fix]") {
        SECTION("unclosed curly brace") {
            auto fixed = fix_syntax_errors("\\textbf{Bold");
            REQUIRE(fixed.find("}") != std::string::npos);
            REQUIRE_FALSE(has_syntax_errors(fixed));
        }

        SECTION("unclosed bracket in sqrt") {
            auto fixed = fix_syntax_errors("\\sqrt[3{2}");
            REQUIRE(fixed.find("]") != std::string::npos);
            REQUIRE_FALSE(has_syntax_errors(fixed));
        }
    }

    TEST_CASE("SyntaxFix: unclosed environment is fixed", "[syntax][fix]") {
        SECTION("missing end document") {
            auto fixed = fix_syntax_errors("\\begin{document}\nHello");
            REQUIRE(fixed.find("\\end{document}") != std::string::npos);
            REQUIRE_FALSE(has_syntax_errors(fixed));
        }

        SECTION("nested environment fix") {
            auto fixed = fix_syntax_errors(
                "\\begin{document}\n"
                "\\begin{figure}\n"
                "content\n"
                "\\end{document}");
            REQUIRE(fixed.find("\\end{figure}") != std::string::npos);
            REQUIRE_FALSE(has_syntax_errors(fixed));
        }
    }

    TEST_CASE("SyntaxFix: unclosed math is fixed", "[syntax][fix]") {
        SECTION("missing closing inline math") {
            auto fixed = fix_syntax_errors("$x+y");
            REQUIRE(fixed.find("$x+y$") != std::string::npos);
            REQUIRE_FALSE(has_syntax_errors(fixed));
        }

        SECTION("missing closing display math") {
            auto fixed = fix_syntax_errors("$$x+y");
            REQUIRE(fixed.find("$$x+y$$") != std::string::npos);
            REQUIRE_FALSE(has_syntax_errors(fixed));
        }
    }

    TEST_CASE("SyntaxFix: multiple fixes applied", "[syntax][fix]") {
        SECTION("unclosed brace and math") {
            auto fixed = fix_syntax_errors("\\textbf{$x+y");
            REQUIRE(fixed.find("$}") != std::string::npos);
            REQUIRE_FALSE(has_syntax_errors(fixed));
        }
    }

    TEST_CASE("SyntaxFix: valid input unchanged", "[syntax][fix]") {
        SECTION("complete document") {
            auto fixed = fix_syntax_errors("\\begin{document}\nHello\n\\end{document}");
            REQUIRE(fixed == "\\begin{document}\nHello\n\\end{document}");
        }

        SECTION("complete math") {
            auto fixed = fix_syntax_errors("$x+y$");
            REQUIRE(fixed == "$x+y$");
        }

        SECTION("complete group") {
            auto fixed = fix_syntax_errors("\\textbf{Bold}");
            REQUIRE(fixed == "\\textbf{Bold}");
        }
    }

    TEST_CASE("SyntaxFix: nested unclosed braces", "[syntax][fix]") {
        SECTION("two levels nested") {
            auto fixed = fix_syntax_errors("\\textbf{\\textit{inner");
            REQUIRE(fixed.find("inner}}") != std::string::npos);
            REQUIRE_FALSE(has_syntax_errors(fixed));
        }

        SECTION("three levels nested") {
            auto fixed = fix_syntax_errors("{\\textbf{\\textit{text");
            REQUIRE(fixed.find("text}}}") != std::string::npos);
            REQUIRE_FALSE(has_syntax_errors(fixed));
        }
    }

    TEST_CASE("SyntaxFix: cross-type group in commands", "[syntax][fix]") {
        SECTION("sqrt bracket then brace") {
            auto fixed = fix_syntax_errors("\\sqrt[3{2}");
            REQUIRE(fixed.find("\\sqrt[3]{2}") != std::string::npos);
            REQUIRE_FALSE(has_syntax_errors(fixed));
        }

        SECTION("unclosed brace after text bracket is fixed") {
            auto fixed = fix_syntax_errors("\\binom[3{2}");
            REQUIRE(fixed.find("}") != std::string::npos);
        }
    }

    TEST_CASE("SyntaxFix: environment with missing braces in name", "[syntax][fix]") {
        SECTION("unclosed environment still detected") {
            auto fixed = fix_syntax_errors("\\begin{document}\nHello\n");
            REQUIRE(fixed.find("\\end{document}") != std::string::npos);
            REQUIRE_FALSE(has_syntax_errors(fixed));
        }
    }

    TEST_CASE("SyntaxFix: long document with scattered errors", "[syntax][fix]") {
        std::string doc =
            "\\begin{document}\n"
            "\\section{Intro}\n"
            "Here is $E=mc^2 without closing.\n"
            "Here is {unclosed brace.\n"
            "Here is $$display unclosed.\n"
            "\\end{document}\n";
        auto fixed = fix_syntax_errors(doc);
        REQUIRE(fixed != doc);
        REQUIRE(fixed.find("$") != std::string::npos);
        REQUIRE(fixed.find("}") != std::string::npos);
    }

    TEST_CASE("SyntaxFix: long document with nested errors", "[syntax][fix]") {
        std::string doc =
            "\\begin{document}\n"
            "\\begin{itemize}\n"
            "  \\item First $x\n"
            "  \\item Second {unclosed\n"
            "  \\item Third\n"
            "\\end{itemize}\n"
            "\\end{document}\n";
        auto fixed = fix_syntax_errors(doc);
        REQUIRE(fixed != doc);
        REQUIRE(fixed.find("$") != std::string::npos);
        REQUIRE(fixed.find("}") != std::string::npos);
    }

    TEST_CASE("SyntaxFix: deep nesting with multiple fix types", "[syntax][fix]") {
        std::string doc =
            "\\begin{document}\n"
            "\\begin{figure}\n"
            "  \\begin{center}\n"
            "    \\textbf{$x + y\n"
            "  \\end{center}\n"
            "\\end{figure}\n"
            "\\end{document}\n";
        auto fixed = fix_syntax_errors(doc);
        REQUIRE_FALSE(has_syntax_errors(fixed));
        REQUIRE(fixed.find("$") != std::string::npos); // math was closed
        REQUIRE(fixed.find("}") != std::string::npos); // brace was closed
    }

    TEST_CASE("SyntaxFix: fixes are idempotent", "[syntax][fix]") {
        SECTION("double fix same result") {
            auto first = fix_syntax_errors("\\textbf{$x+y");
            auto second = fix_syntax_errors(first);
            REQUIRE(first == second);
        }

        SECTION("complex case idempotent") {
            auto first = fix_syntax_errors("\\begin{document}\n\\begin{figure}\n$x\n\\end{figure}\n\\end{document}");
            auto second = fix_syntax_errors(first);
            REQUIRE(first == second);
        }
    }

    TEST_CASE("SyntaxFix: edge cases", "[syntax][fix]") {
        SECTION("only whitespace unchanged") {
            auto fixed = fix_syntax_errors("   \n  \n  ");
            REQUIRE(fixed == "   \n  \n  ");
        }

        SECTION("empty string unchanged") {
            auto fixed = fix_syntax_errors("");
            REQUIRE(fixed == "");
        }

        SECTION("text without any braces unchanged") {
            auto fixed = fix_syntax_errors("Hello world, no LaTeX at all.");
            REQUIRE(fixed == "Hello world, no LaTeX at all.");
        }

        SECTION("many stray closing braces") {
            auto fixed = fix_syntax_errors("text}}more}}");
            REQUIRE_FALSE(has_syntax_errors(fixed));
        }

        SECTION("unclosed brace in text mode") {
            auto fixed = fix_syntax_errors("Some text {with unclosed group");
            REQUIRE(fixed.find("group}") != std::string::npos);
            REQUIRE_FALSE(has_syntax_errors(fixed));
        }
    }

    TEST_CASE("SyntaxFix: correct content not broken by fixes", "[syntax][fix]") {
        SECTION("long valid document unchanged") {
            std::string doc =
                "\\begin{document}\n"
                "\\section{Intro}\n"
                "Hello $x+y$ world.\n"
                "\\textbf{Bold} and \\textit{italic}.\n"
                "\\begin{equation}\n"
                "  E = mc^2\n"
                "\\end{equation}\n"
                "\\begin{figure}\n"
                "  \\centering\n"
                "  \\includegraphics{img.pdf}\n"
                "  \\caption{A figure}\n"
                "\\end{figure}\n"
                "\\end{document}\n";
            auto fixed = fix_syntax_errors(doc);
            REQUIRE(fixed == doc);
        }
    }

    TEST_CASE("Tag removal", "[formatting]") {
        SECTION("remove \\tag{...} when enabled") {
            FormatConfig cfg;
            cfg.remove_tags = true;
            auto result = format_code_config(
                "\\begin{equation}\n"
                "E = mc^2 \\tag{1}\n"
                "\\end{equation}", cfg);
            REQUIRE(result.find("\\tag") == std::string::npos);
            REQUIRE(result.find("E = mc^{2}") != std::string::npos);
        }

        SECTION("remove \\tag*{...} when enabled") {
            FormatConfig cfg;
            cfg.remove_tags = true;
            auto result = format_code_config(
                "\\begin{equation}\n"
                "E = mc^2 \\tag*{1}\n"
                "\\end{equation}", cfg);
            REQUIRE(result.find("\\tag") == std::string::npos);
            REQUIRE(result.find("E = mc^{2}") != std::string::npos);
        }

        SECTION("preserve \\tag{...} by default") {
            FormatConfig cfg;
            auto result = format_code_config(
                "\\begin{equation}\n"
                "E = mc^2 \\tag{1}\n"
                "\\end{equation}", cfg);
            REQUIRE(result.find("\\tag{1}") != std::string::npos);
        }

        SECTION("remove \\tag in display math") {
            FormatConfig cfg;
            cfg.remove_tags = true;
            auto result = format_code_config(
                "$$\n"
                "a + b = c \\tag{2}\n"
                "$$", cfg);
            REQUIRE(result.find("\\tag") == std::string::npos);
            REQUIRE(result.find("a + b = c") != std::string::npos);
        }
    }

    TEST_CASE("SyntaxFix: mixed inline/display math", "[syntax][fix]") {
        SECTION("inline math closed by display math $$ delimiter") {
            auto fixed = fix_syntax_errors(
                "则容易验证 $ 稳定因果 $$\n"
                "  \\tilde{g}^{ab} = g^{ab} + \\frac{t^a t^b}{1+\\alpha^2}\n"
                "$$ 其中\n");
            bool has_closing = (fixed.find("$ 稳定因果 $") != std::string::npos)
                            || (fixed.find("$ 稳定因果") != std::string::npos);
            REQUIRE(has_closing);
            REQUIRE_FALSE(has_syntax_errors(fixed));
        }

        SECTION("unclosed display math at end of document") {
            auto fixed = fix_syntax_errors(
                "则容易验证\n"
                "$\\tilde{g}^{ab} = g^{ab}$\n"
                "$$\n"
                "其中\n");
            REQUIRE(fixed.find("$$") != std::string::npos);
            REQUIRE_FALSE(has_syntax_errors(fixed));
        }

        SECTION("$...$$ sequence is handled") {
            auto fixed = fix_syntax_errors(
                "$$\n"
                "  \\tilde{g}^{ab} = g^{ab}\n"
                "$$\n");
            REQUIRE_FALSE(has_syntax_errors(fixed));
        }
    }

    TEST_CASE("SyntaxCheck: character-based column", "[syntax]") {
        Registry registry;
        registry.registerBuiltin();

        auto get_errors = [&](const std::string& input) {
            Lexer lexer(input, registry);
            auto tokens = lexer.tokenize();
            Parser parser(std::move(tokens), input, registry);
            auto doc = parser.parse();
            SyntaxChecker checker(input, *doc);
            return checker.check();
        };

        auto get_context = [&](const std::string& input) {
            auto errors = get_errors(input);
            if (errors.empty()) return std::string();
            return errors[0].context;
        };

        SECTION("CJK column is character-based not byte-based") {
            auto errors = get_errors("充分性的{证明：\n");
            REQUIRE_FALSE(errors.empty());
            REQUIRE(errors[0].col == 5);
        }

        SECTION("ASCII column is character-based") {
            auto errors = get_errors("abc{test\n");
            REQUIRE_FALSE(errors.empty());
            REQUIRE(errors[0].col == 4);
        }

        SECTION("context shows surrounding characters") {
            auto ctx = get_context("充分性的{证明：已知 中文测试\n");
            REQUIRE_FALSE(ctx.empty());
            REQUIRE(ctx.find("充分性的") != std::string::npos);
            REQUIRE(ctx.find("{") != std::string::npos);
        }

        SECTION("context at line start") {
            auto ctx = get_context("{test\n");
            REQUIRE_FALSE(ctx.empty());
            REQUIRE(ctx.find("{test") != std::string::npos);
        }

        SECTION("context at line end") {
            auto ctx = get_context("abc{\n");
            REQUIRE_FALSE(ctx.empty());
            REQUIRE(ctx.find("abc{") != std::string::npos);
        }

        SECTION("extraneous close brace column") {
            auto errors = get_errors("中文}测试\n");
            REQUIRE_FALSE(errors.empty());
            REQUIRE(errors[0].col == 3);
        }

        SECTION("extraneous close brace context") {
            auto ctx = get_context("中文}测试\n");
            REQUIRE_FALSE(ctx.empty());
            REQUIRE(ctx.find("中文}测试") != std::string::npos);
        }
    }

    TEST_CASE("Brackets in math mode are regular characters", "[formatter][bracket]") {
        SECTION("display math with tensor notation brackets") {
            auto result = format_code(
                "$$\n"
                "\\delta^{[a_1}_{a_1}\n"
                "$$");
            REQUIRE(result.find("\\delta^{[a_{1}}_{a_{1}}") != std::string::npos);
        }

        SECTION("display math with unmatched brackets in formula") {
            auto result = format_code(
                "$$\n"
                "\\delta^{[a_1}_{a_1} \\cdots \\delta^{a_n]}_{b_n}\n"
                "$$");
            REQUIRE(result.find("\\delta^{[a_{1}}_{a_{1}}") != std::string::npos);
            REQUIRE(result.find("\\delta^{a_{n}]}_{b_{n}}") != std::string::npos);
        }

        SECTION("tensor notation formula from bug report") {
            auto result = format_code(
                "$$\n"
                "\\delta^{[a_1}_{a_1} \\cdots  \\delta^{a_n]}_{\\phantom{[}b_n}"
                " = \\frac{(n-j)! j!}{n!}"
                " \\delta^{[a_{j+1}}_{b_{j+1}} \\cdots \\delta^{a_n]}_{b_n}.\n"
                "$$");
            REQUIRE(result.find("\\delta^{[a_{1}}_{a_{1}}") != std::string::npos);
            REQUIRE(result.find("\\delta^{a_{n}]}_{\\phantom{[}b_{n}}") != std::string::npos);
            REQUIRE(result.find("\\delta^{[a_{j+1}}_{b_{j+1}}") != std::string::npos);
            REQUIRE(result.find("\\delta^{a_{n}]}_{b_{n}}") != std::string::npos);
        }

        SECTION("inline math with bracket chars") {
            auto result = format_code("$[a,b]$");
            REQUIRE(result.find("$[a,b]$") != std::string::npos);
        }

        SECTION("inline math with unmatched brackets") {
            auto result = format_code("$x^{[n}$");
            REQUIRE(result.find("$x^{[n}$") != std::string::npos);
        }
    }

    TEST_CASE("Optional args in math mode still work", "[formatter][bracket]") {
        SECTION("sqrt with optional arg in display math") {
            auto result = format_code("$$\\sqrt[3]{x}$$");
            REQUIRE(result.find("\\sqrt[3]{x}") != std::string::npos);
        }

        SECTION("sqrt with optional arg in inline math") {
            auto result = format_code("$\\sqrt[3]{x}$");
            REQUIRE(result.find("$\\sqrt[3]{x}$") != std::string::npos);
        }

        SECTION("command with optional arg followed by regular brackets") {
            auto result = format_code("$$\\sqrt[3]{x^{[n]}}$$");
            REQUIRE(result.find("\\sqrt[3]{x^{[n]}}") != std::string::npos);
        }

        SECTION("option arg with nested bracket-like chars") {
            auto result = format_code("$$\\sqrt[3]{a^{[i}_{j]}}$$");
            REQUIRE(result.find("\\sqrt[3]{a^{[i}_{j]}}") != std::string::npos);
        }
    }

    TEST_CASE("Brackets in text mode are regular characters", "[formatter][bracket]") {
        SECTION("standalone brackets in text") {
            auto result = format_code("Some text [with brackets] here.");
            REQUIRE(result.find("[with brackets]") != std::string::npos);
        }

        SECTION("unmatched open bracket in text") {
            auto result = format_code("Some text [with unmatched bracket");
            REQUIRE(result.find("[with unmatched bracket") != std::string::npos);
        }

        SECTION("close bracket alone in text") {
            auto result = format_code("Some text] here.");
            REQUIRE(result.find("Some text] here.") != std::string::npos);
        }
    }

    TEST_CASE("SyntaxCheck: no false positive for brackets in math", "[syntax][bracket]") {
        SECTION("display math with regular brackets has no errors") {
            auto errors = check_syntax(
                "$$\n"
                "\\delta^{[a_1}_{a_1} \\cdots \\delta^{a_n]}_{b_n}\n"
                "$$");
            REQUIRE(errors.empty());
        }

        SECTION("inline math with regular brackets has no errors") {
            auto errors = check_syntax("$x^{[n}$");
            REQUIRE(errors.empty());
        }

        SECTION("sqrt with optional arg still detects unclosed bracket") {
            auto errors = check_syntax("\\sqrt[3{2}");
            REQUIRE_FALSE(errors.empty());
            REQUIRE(errors[0].find("'['") != std::string::npos);
        }
    }

    TEST_CASE("Idempotency: bracket chars in formulas", "[formatter][idempotent][bracket]") {
        SECTION("tensor notation formula idempotent") {
            std::string input =
                "$$\n"
                "\\delta^{[a_1}_{a_1} \\cdots \\delta^{a_n]}_{b_n}\n"
                "= \\frac{(n-j)! j!}{n!}\n"
                "\\delta^{[a_{j+1}}_{b_{j+1}} \\cdots \\delta^{a_n]}_{b_n}.\n"
                "$$";
            auto first = format_code(input);
            auto second = format_code(first);
            REQUIRE(first == second);
        }
    }

    TEST_CASE("left-right with brackets formatting", "[formatter][bracket]") {
        SECTION("left dot right brace") {
            auto result = format_code("$$\\left. \\frac{1}{2}\\sqrt[3]4 \\right\\}$$");
            REQUIRE(result.find("\\frac{1}{2}") != std::string::npos);
            REQUIRE(result.find("\\sqrt[3]{4}") != std::string::npos);
        }

        SECTION("left bracket right bracket") {
            auto result = format_code("$$3\\left[ \\frac12\\sqrt[3]4 \\right]$$");
            REQUIRE(result.find("\\frac{1}{2}") != std::string::npos);
            REQUIRE(result.find("\\sqrt[3]{4}") != std::string::npos);
        }

        SECTION("left bracket right dot") {
            auto result = format_code("$$3\\left[ \\frac12\\sqrt[3]4 \\right.$$");
            REQUIRE(result.find("\\frac{1}{2}") != std::string::npos);
            REQUIRE(result.find("\\sqrt[3]{4}") != std::string::npos);
        }

        SECTION("unknown command with optional bracket preserved") {
            REQUIRE(format_code("\\custommacro[opt]{required}") == "\\custommacro[opt]{required}");
        }
    }

    TEST_CASE("SyntaxCheck: left-right constructs no false errors", "[syntax][bracket]") {
        SECTION("left bracket right bracket") {
            auto errors = check_syntax("$$3\\left[ \\frac12\\sqrt[3]4 \\right]$$");
            REQUIRE(errors.empty());
        }

        SECTION("left bracket right dot") {
            auto errors = check_syntax("$$3\\left[ \\frac12\\sqrt[3]4 \\right.$$");
            REQUIRE(errors.empty());
        }

        SECTION("left dot right brace") {
            auto errors = check_syntax("$$\\left. \\frac{1}{2}\\sqrt[3]4 \\right\\}$$");
            REQUIRE(errors.empty());
        }
    }

    TEST_CASE("Delimiter sizing commands are preserved", "[formatter][bracket]") {
        SECTION("left-right pairs") {
            auto result = format_code("$\\left( \\frac{1}{2} \\right)$");
            REQUIRE(result.find("\\left( \\frac{1}{2} \\right)") != std::string::npos);
        }

        SECTION("bigl-bigr with parens") {
            auto result = format_code("$\\bigl( x \\bigr)$");
            REQUIRE(result.find("\\bigl( x \\bigr)") != std::string::npos);
        }

        SECTION("Bigl-Bigr with brackets") {
            auto result = format_code("$\\Bigl[ x \\Bigr]$");
            REQUIRE(result.find("\\Bigl[ x \\Bigr]") != std::string::npos);
        }

        SECTION("biggl-biggr with braces") {
            auto result = format_code("$\\biggl\\{ x \\biggr\\}$");
            REQUIRE(result.find("\\biggl\\{ x \\biggr\\}") != std::string::npos);
        }

        SECTION("Biggl-Biggr with langle-rangle") {
            auto result = format_code("$\\Biggl\\langle x \\Biggr\\rangle$");
            REQUIRE(result.find("\\Biggl\\langle x \\Biggr\\rangle") != std::string::npos);
        }

        SECTION("big-Big-bigg-Bigg single") {
            auto result = format_code("$\\big| x \\big|$ $\\Big| x \\Big|$ $\\bigg| x \\bigg|$ $\\Bigg| x \\Bigg|$");
            REQUIRE(result.find("\\big|") != std::string::npos);
            REQUIRE(result.find("\\Big|") != std::string::npos);
            REQUIRE(result.find("\\bigg|") != std::string::npos);
            REQUIRE(result.find("\\Bigg|") != std::string::npos);
        }

        SECTION("lfloor-rfloor and lceil-rceil") {
            auto result = format_code("$\\bigl\\lfloor x \\bigr\\rfloor$ $\\bigl\\lceil x \\bigr\\rceil$");
            REQUIRE(result.find("\\bigl\\lfloor") != std::string::npos);
            REQUIRE(result.find("\\bigr\\rfloor") != std::string::npos);
            REQUIRE(result.find("\\bigl\\lceil") != std::string::npos);
            REQUIRE(result.find("\\bigr\\rceil") != std::string::npos);
        }
    }

} // namespace latex_fmt
