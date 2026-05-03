#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <sstream>
#include <fstream>
#include <filesystem>

#ifndef LATEX_FMT_BIN
#error "LATEX_FMT_BIN not defined — CMake should set it to the latex-fmt binary path"
#endif

namespace {

struct CmdResult {
    std::string output;
    int exit_code;
};

CmdResult run_cmd(const std::string& cmd) {
    std::string full_cmd = std::string(LATEX_FMT_BIN) + " " + cmd + " 2>&1";
    std::ostringstream out;
#ifdef _WIN32
    FILE* pipe = _popen(full_cmd.c_str(), "r");
#else
    FILE* pipe = popen(full_cmd.c_str(), "r");
#endif
    if (!pipe) return {"", -1};
    char buf[4096];
    while (fgets(buf, sizeof(buf), pipe)) {
        out << buf;
    }
#ifdef _WIN32
    int ec = _pclose(pipe);
#else
    int ec = pclose(pipe);
#endif
    int exit_code = (ec == -1) ? -1 : WEXITSTATUS(ec);
    return {out.str(), exit_code};
}

void write_file(const std::string& path, const std::string& content) {
    std::filesystem::create_directories(std::filesystem::path(path).parent_path());
    std::ofstream f(path);
    f << content;
}

std::string read_file(const std::string& path) {
    std::ifstream f(path);
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

std::string tmp_path(const std::string& name) {
    return (std::filesystem::temp_directory_path() / ("latexfmt_" + name)).string();
}

} // namespace

TEST_CASE("CLI: --help", "[cli]") {
    SECTION("--help shows usage") {
        auto r = run_cmd("--help");
        REQUIRE(r.exit_code == 0);
        REQUIRE_THAT(r.output, Catch::Matchers::ContainsSubstring("Usage:"));
        REQUIRE_THAT(r.output, Catch::Matchers::ContainsSubstring("--help"));
        REQUIRE_THAT(r.output, Catch::Matchers::ContainsSubstring("--indent-width"));
    }
    SECTION("-h shortcut works") {
        auto r = run_cmd("-h");
        REQUIRE(r.exit_code == 0);
        REQUIRE_THAT(r.output, Catch::Matchers::ContainsSubstring("Usage:"));
    }
}

TEST_CASE("CLI: --version", "[cli]") {
    SECTION("--version shows version") {
        auto r = run_cmd("--version");
        REQUIRE(r.exit_code == 0);
        REQUIRE_THAT(r.output, Catch::Matchers::ContainsSubstring("latex-fmt v"));
    }
    SECTION("-V shortcut works") {
        auto r = run_cmd("-V");
        REQUIRE(r.exit_code == 0);
        REQUIRE_THAT(r.output, Catch::Matchers::ContainsSubstring("latex-fmt v"));
    }
}

TEST_CASE("CLI: unknown option", "[cli]") {
    auto r = run_cmd("--unknown-flag");
    REQUIRE(r.exit_code == 1);
    REQUIRE_THAT(r.output, Catch::Matchers::ContainsSubstring("unknown option"));
}

TEST_CASE("CLI: file argument", "[cli]") {
    std::string f = tmp_path("file_arg.tex");
    write_file(f, "\\frac12\n");

    SECTION("reads from file and writes to stdout") {
        auto r = run_cmd(f);
        REQUIRE(r.exit_code == 0);
        REQUIRE_THAT(r.output, Catch::Matchers::ContainsSubstring("\\frac{1}{2}"));
    }

    std::filesystem::remove(f);
}

TEST_CASE("CLI: -o output file", "[cli]") {
    std::string in = tmp_path("output_in.tex");
    std::string out = tmp_path("output_out.tex");
    write_file(in, "\\frac12\n");

    auto r = run_cmd("-o " + out + " " + in);
    REQUIRE(r.exit_code == 0);
    REQUIRE_THAT(read_file(out), Catch::Matchers::ContainsSubstring("\\frac{1}{2}"));

    std::filesystem::remove(in);
    std::filesystem::remove(out);
}

TEST_CASE("CLI: -i in-place", "[cli]") {
    std::string f = tmp_path("inplace.tex");
    write_file(f, "\\frac12\n");

    auto r = run_cmd("-i " + f);
    REQUIRE(r.exit_code == 0);
    REQUIRE_THAT(read_file(f), Catch::Matchers::ContainsSubstring("\\frac{1}{2}"));

    std::filesystem::remove(f);
}

TEST_CASE("CLI: --check mode", "[cli]") {
    std::string needs = tmp_path("check_needs.tex");
    std::string ok = tmp_path("check_ok.tex");
    write_file(needs, "\\frac12\nhello  world\n");
    write_file(ok, "\\frac{1}{2}\nhello world\n");

    SECTION("needs formatting → exit 1") {
        auto r = run_cmd("--check " + needs);
        REQUIRE(r.exit_code == 1);
        REQUIRE_THAT(r.output, Catch::Matchers::ContainsSubstring("needs formatting"));
    }

    SECTION("already formatted → exit 0") {
        auto r = run_cmd("--check " + ok);
        REQUIRE(r.exit_code == 0);
    }

    SECTION("stdin needs formatting → exit 1") {
        std::string stdin_file = tmp_path("stdin_check.tex");
        write_file(stdin_file, "\\frac12\n");
        std::string cmd = std::string(LATEX_FMT_BIN) + " --check < " + stdin_file + " 2>&1";
        FILE* p = popen(cmd.c_str(), "r");
        std::ostringstream oss;
        char buf[4096];
        while (fgets(buf, sizeof(buf), p)) oss << buf;
        int ec = pclose(p);
        int code = WEXITSTATUS(ec);
        REQUIRE(code == 1);
        REQUIRE_THAT(oss.str(), Catch::Matchers::ContainsSubstring("needs formatting"));
        std::filesystem::remove(stdin_file);
    }

    std::filesystem::remove(needs);
    std::filesystem::remove(ok);
}

TEST_CASE("CLI: --diff mode", "[cli]") {
    std::string f = tmp_path("diff_in.tex");
    write_file(f, "\\frac12\n");

    SECTION("diff output contains unified diff markers") {
        auto r = run_cmd("--diff " + f);
        REQUIRE(r.exit_code == 1);
        REQUIRE_THAT(r.output, Catch::Matchers::ContainsSubstring("---"));
        REQUIRE_THAT(r.output, Catch::Matchers::ContainsSubstring("+++"));
        REQUIRE_THAT(r.output, Catch::Matchers::ContainsSubstring("@@"));
        REQUIRE_THAT(r.output, Catch::Matchers::ContainsSubstring("-\\frac12"));
        REQUIRE_THAT(r.output, Catch::Matchers::ContainsSubstring("+\\frac{1}{2}"));
    }

    SECTION("already formatted → empty diff, exit 0") {
        std::string ok = tmp_path("diff_ok.tex");
        write_file(ok, "\\frac{1}{2}\n");
        auto r = run_cmd("--diff " + ok);
        REQUIRE(r.exit_code == 0);
        std::filesystem::remove(ok);
    }

    std::filesystem::remove(f);
}

TEST_CASE("CLI: --quiet", "[cli]") {
    std::string f = tmp_path("quiet.tex");
    write_file(f, "\\frac12 \\frac12 \\frac12 \\frac12 \\frac12 \\frac12 \\frac12 \\frac12 \\frac12 \\frac12\n");

    SECTION("without --quiet shows warnings") {
        auto r = run_cmd("--max-line-width=40 " + f);
        REQUIRE_THAT(r.output, Catch::Matchers::ContainsSubstring("WARNING:"));
    }

    SECTION("with --quiet suppresses warnings") {
        auto r = run_cmd("--max-line-width=40 --quiet " + f);
        REQUIRE(r.output.find("WARNING:") == std::string::npos);
    }

    SECTION("-q shortcut suppresses warnings") {
        auto r = run_cmd("--max-line-width=40 -q " + f);
        REQUIRE(r.output.find("WARNING:") == std::string::npos);
    }

    std::filesystem::remove(f);
}

TEST_CASE("CLI: --indent-width", "[cli]") {
    std::string f = tmp_path("indent.tex");
    write_file(f, "\\documentclass{article}\n\\begin{document}\n\\begin{figure}\nhello\n\\end{figure}\n\\end{document}\n");

    SECTION("default indent is 2") {
        auto r = run_cmd(f);
        REQUIRE_THAT(r.output, Catch::Matchers::ContainsSubstring("  hello"));
    }

    SECTION("custom indent 4") {
        auto r = run_cmd("--indent-width=4 " + f);
        REQUIRE_THAT(r.output, Catch::Matchers::ContainsSubstring("    hello"));
    }

    std::filesystem::remove(f);
}

TEST_CASE("CLI: --no-cjk-spacing", "[cli]") {
    std::string f = tmp_path("cjk.tex");
    write_file(f, "\\section{你好World}\n");

    SECTION("default adds space") {
        auto r = run_cmd(f);
        REQUIRE_THAT(r.output, Catch::Matchers::ContainsSubstring("你好 World"));
    }

    SECTION("--no-cjk-spacing preserves no space") {
        auto r = run_cmd("--no-cjk-spacing " + f);
        REQUIRE_THAT(r.output, Catch::Matchers::ContainsSubstring("你好World"));
    }

    SECTION("--cjk-spacing re-enables") {
        auto r = run_cmd("--cjk-spacing " + f);
        REQUIRE_THAT(r.output, Catch::Matchers::ContainsSubstring("你好 World"));
    }

    std::filesystem::remove(f);
}

TEST_CASE("CLI: --no-brace-completion", "[cli]") {
    std::string f = tmp_path("brace.tex");
    write_file(f, "\\frac12\n");

    SECTION("default adds braces") {
        auto r = run_cmd(f);
        REQUIRE_THAT(r.output, Catch::Matchers::ContainsSubstring("\\frac{1}{2}"));
    }

    SECTION("--no-brace-completion preserves shorthand") {
        auto r = run_cmd("--no-brace-completion " + f);
        REQUIRE_THAT(r.output, Catch::Matchers::ContainsSubstring("\\frac12"));
    }

    SECTION("--brace-completion re-enables") {
        auto r = run_cmd("--brace-completion " + f);
        REQUIRE_THAT(r.output, Catch::Matchers::ContainsSubstring("\\frac{1}{2}"));
    }

    std::filesystem::remove(f);
}

TEST_CASE("CLI: --no-comment-normalize", "[cli]") {
    SECTION("default normalizes comment") {
        std::string f = tmp_path("cmt_default.tex");
        write_file(f, "%comment\n");
        auto r = run_cmd(f);
        REQUIRE_THAT(r.output, Catch::Matchers::ContainsSubstring("% comment"));
        std::filesystem::remove(f);
    }

    SECTION("--no-comment-normalize preserves raw") {
        std::string f = tmp_path("cmt_raw.tex");
        write_file(f, "%comment\n");
        auto r = run_cmd("--no-comment-normalize " + f);
        REQUIRE_THAT(r.output, Catch::Matchers::ContainsSubstring("%comment"));
        std::filesystem::remove(f);
    }
}

TEST_CASE("CLI: --keep-trailing-spaces", "[cli]") {
    std::string f = tmp_path("trail.tex");
    write_file(f, "hello \nworld\n");

    SECTION("default removes trailing spaces") {
        auto r = run_cmd(f);
        REQUIRE(r.output.find("hello ") == std::string::npos);
    }

    SECTION("--keep-trailing-spaces preserves them") {
        auto r = run_cmd("--keep-trailing-spaces " + f);
        REQUIRE(r.output.find("hello ") != std::string::npos);
    }

    std::filesystem::remove(f);
}

TEST_CASE("CLI: --no-blank-line-compress", "[cli]") {
    std::string f = tmp_path("blank.tex");
    write_file(f, "a\n\n\n\nb\n");

    SECTION("default compresses blanks") {
        auto r = run_cmd(f);
        REQUIRE(r.output.find("a\n\nb") != std::string::npos);
    }

    SECTION("flag is accepted and produces different output") {
        auto r_default = run_cmd(f);
        auto r_no = run_cmd("--no-blank-line-compress " + f);
        REQUIRE(r_default.output != r_no.output);
    }

    std::filesystem::remove(f);
}

TEST_CASE("CLI: --no-display-math-format", "[cli]") {
    std::string f = tmp_path("dispmath.tex");
    write_file(f, "$$x+y$$\n");

    SECTION("default puts math on separate lines") {
        auto r = run_cmd(f);
        REQUIRE_THAT(r.output, Catch::Matchers::ContainsSubstring("$$\n  x+y\n$$"));
    }

    SECTION("--no-display-math-format keeps inline") {
        auto r = run_cmd("--no-display-math-format " + f);
        REQUIRE_THAT(r.output, Catch::Matchers::ContainsSubstring("$$x+y$$"));
    }

    std::filesystem::remove(f);
}

TEST_CASE("CLI: --no-math-unify", "[cli]") {
    std::string f = tmp_path("mathuni.tex");
    write_file(f, "\\(x+y\\)\n");

    SECTION("default converts to $") {
        auto r = run_cmd(f);
        REQUIRE_THAT(r.output, Catch::Matchers::ContainsSubstring("$x+y$"));
    }

    SECTION("--no-math-unify preserves \\( \\)") {
        auto r = run_cmd("--no-math-unify " + f);
        REQUIRE_THAT(r.output, Catch::Matchers::ContainsSubstring("\\(x+y\\)"));
    }

    std::filesystem::remove(f);
}

TEST_CASE("CLI: --max-line-width warning", "[cli]") {
    std::string f = tmp_path("long.tex");
    write_file(f, "\\frac12\\frac12\\frac12\\frac12\\frac12\\frac12\\frac12\\frac12\\frac12\\frac12\n");

    SECTION("warns when line exceeds width") {
        auto r = run_cmd("--max-line-width=30 " + f);
        REQUIRE(r.exit_code == 0);
        REQUIRE_THAT(r.output, Catch::Matchers::ContainsSubstring("WARNING"));
        REQUIRE_THAT(r.output, Catch::Matchers::ContainsSubstring("exceeds max width 30"));
    }

    SECTION("no warning when width is 0") {
        auto r = run_cmd("--max-line-width=0 " + f);
        REQUIRE(r.output.find("WARNING:") == std::string::npos);
    }

    std::filesystem::remove(f);
}

TEST_CASE("CLI: --wrap comment line", "[cli]") {
    std::string f = tmp_path("cmt_wrap.tex");
    std::string long_comment = "% " + std::string(70, 'x') + " more text here to wrap it properly xyz\n";
    write_file(f, long_comment);

    SECTION("--wrap wraps long comment lines") {
        auto r = run_cmd("--max-line-width=40 --wrap " + f);
        REQUIRE(r.exit_code == 0);
        auto s = r.output;
        bool wrapped = (s.find("% \n") != std::string::npos || s.find("%\n") != std::string::npos);
        REQUIRE(wrapped);
    }

    std::filesystem::remove(f);
}

TEST_CASE("CLI: --wrap-paragraphs", "[cli]") {
    std::string f = tmp_path("para_wrap.tex");
    std::string long_text = "This is a test of wrapping. More text here to make it long enough to wrap properly yes.\n";
    write_file(f, long_text);

    SECTION("--wrap-paragraphs wraps long paragraph lines") {
        auto r = run_cmd("--max-line-width=40 --wrap-paragraphs " + f);
        REQUIRE(r.exit_code == 0);
        REQUIRE_THAT(r.output, Catch::Matchers::ContainsSubstring("\n  "));
    }

    SECTION("without --wrap-paragraphs, long text stays on one line") {
        auto r = run_cmd("--max-line-width=40 " + f);
        REQUIRE_THAT(r.output, Catch::Matchers::ContainsSubstring("WARNING"));
    }

    std::filesystem::remove(f);
}

TEST_CASE("CLI: --wrap all line types", "[cli]") {
    std::string f = tmp_path("wrap_all.tex");
    std::string long_text = "This is a very long line of text that should be wrapped when wrap mode is active and the line is long enough.\n";
    write_file(f, long_text);

    SECTION("--wrap wraps non-comment lines") {
        auto r = run_cmd("--max-line-width=40 --wrap " + f);
        REQUIRE(r.exit_code == 0);
        REQUIRE_THAT(r.output, Catch::Matchers::ContainsSubstring("\n"));
    }

    std::filesystem::remove(f);
}

TEST_CASE("CLI: --wrap-paragraphs with inline math", "[cli]") {
    std::string f = tmp_path("wrap_math.tex");
    std::string text = "This long paragraph has inline math $x=y$ and should still be wrapped into multiple lines when exceeding max width.\n";
    write_file(f, text);

    SECTION("--wrap-paragraphs wraps prose lines containing inline $") {
        auto r = run_cmd("--max-line-width=40 --wrap-paragraphs " + f);
        REQUIRE(r.exit_code == 0);
        REQUIRE_THAT(r.output, Catch::Matchers::ContainsSubstring("\n"));
    }

    std::filesystem::remove(f);
}

TEST_CASE("CLI: --config-file", "[cli]") {
    std::string rc = tmp_path("testrc");
    write_file(rc, "cjk_spacing = false\nbrace_completion = false\nindent_width = 4\n");
    std::string f = tmp_path("cfgin.tex");
    write_file(f, "\\section{你好World}\n\\frac12\n\\begin{figure}\nhi\n\\end{figure}\n");

    SECTION("config file applies settings") {
        auto r = run_cmd("--config-file=" + rc + " " + f);
        REQUIRE(r.exit_code == 0);
        REQUIRE_THAT(r.output, Catch::Matchers::ContainsSubstring("你好World"));
        REQUIRE_THAT(r.output, Catch::Matchers::ContainsSubstring("\\frac12"));
        REQUIRE_THAT(r.output, Catch::Matchers::ContainsSubstring("    hi"));
    }

    SECTION("CLI overrides config file") {
        auto r = run_cmd("--config-file=" + rc + " --cjk-spacing --brace-completion " + f);
        REQUIRE(r.exit_code == 0);
        REQUIRE_THAT(r.output, Catch::Matchers::ContainsSubstring("你好 World"));
        REQUIRE_THAT(r.output, Catch::Matchers::ContainsSubstring("\\frac{1}{2}"));
    }

    std::filesystem::remove(rc);
    std::filesystem::remove(f);
}

TEST_CASE("CLI: -r recursive", "[cli]") {
    std::string dir = tmp_path("recdir");
    std::filesystem::create_directories(dir + "/sub");
    write_file(dir + "/a.tex", "\\frac12\n");
    write_file(dir + "/sub/b.tex", "\\frac12\n");
    write_file(dir + "/not.abc", "\\frac12\n");

    SECTION("-r --check finds .tex files") {
        auto r = run_cmd("-r --check " + dir);
        REQUIRE(r.exit_code == 1);
        REQUIRE_THAT(r.output, Catch::Matchers::ContainsSubstring("needs formatting"));
        REQUIRE_THAT(r.output, Catch::Matchers::ContainsSubstring("2 of 2"));
    }

    SECTION("-r -i formats in place") {
        auto r = run_cmd("-r -i " + dir);
        REQUIRE(r.exit_code == 0);
        REQUIRE_THAT(read_file(dir + "/a.tex"), Catch::Matchers::ContainsSubstring("\\frac{1}{2}"));
        REQUIRE_THAT(read_file(dir + "/sub/b.tex"), Catch::Matchers::ContainsSubstring("\\frac{1}{2}"));
        REQUIRE_THAT(read_file(dir + "/not.abc"), Catch::Matchers::ContainsSubstring("\\frac12"));
    }

    std::filesystem::remove_all(dir);
}

TEST_CASE("CLI: error handling", "[cli]") {
    SECTION("-i and -o cannot be used together") {
        auto r = run_cmd("-i -o out.tex in.tex 2>&1");
        REQUIRE(r.exit_code == 1);
    }

    SECTION("--check cannot be used with -i") {
        auto r = run_cmd("--check -i in.tex 2>&1");
        REQUIRE(r.exit_code == 1);
    }

    SECTION("--diff cannot be used with -o") {
        auto r = run_cmd("--diff -o out.tex in.tex 2>&1");
        REQUIRE(r.exit_code == 1);
    }

    SECTION("-r cannot be used with -o") {
        auto r = run_cmd("-r -o out.tex dir 2>&1");
        REQUIRE(r.exit_code == 1);
    }

    SECTION("missing file errors gracefully") {
        auto r = run_cmd("/nonexistent/file.tex");
        REQUIRE(r.exit_code == 1);
        REQUIRE_THAT(r.output, Catch::Matchers::ContainsSubstring("cannot read"));
    }
}

TEST_CASE("CLI: --syntax-check option", "[cli][syntax]") {
    SECTION("valid document passes") {
        auto f = tmp_path("valid.tex");
        write_file(f, "\\begin{document}\nHello\n\\end{document}\n");
        auto r = run_cmd("--syntax-check " + f);
        REQUIRE(r.exit_code == 0);
        std::filesystem::remove(f);
    }

    SECTION("unclosed environment fails") {
        auto f = tmp_path("unclosed.tex");
        write_file(f, "\\begin{document}\nHello\n");
        auto r = run_cmd("--syntax-check " + f);
        REQUIRE(r.exit_code == 1);
        REQUIRE_THAT(r.output, Catch::Matchers::ContainsSubstring("has no matching"));
        std::filesystem::remove(f);
    }

    SECTION("unclosed math fails") {
        auto f = tmp_path("unclosed_math.tex");
        write_file(f, "$x+y\n");
        auto r = run_cmd("--syntax-check " + f);
        REQUIRE(r.exit_code == 1);
        REQUIRE_THAT(r.output, Catch::Matchers::ContainsSubstring("inline math"));
        std::filesystem::remove(f);
    }

    SECTION("extraneous brace fails") {
        auto f = tmp_path("extraneous.tex");
        write_file(f, "}text\n");
        auto r = run_cmd("--syntax-check " + f);
        REQUIRE(r.exit_code == 1);
        REQUIRE_THAT(r.output, Catch::Matchers::ContainsSubstring("extraneous"));
        std::filesystem::remove(f);
    }

    SECTION("without --syntax-check, errors are not reported") {
        auto f = tmp_path("no_check.tex");
        write_file(f, "\\begin{document}\n");
        auto r = run_cmd(f);
        REQUIRE(r.exit_code == 0);
        REQUIRE_THAT(r.output, !Catch::Matchers::ContainsSubstring("has no matching"));
        std::filesystem::remove(f);
    }

    SECTION("--syntax-check with stderr output") {
        auto f = tmp_path("stderr_test.tex");
        write_file(f, "$x+y\n");
        auto r = run_cmd("--syntax-check " + f);
        REQUIRE(r.exit_code == 1);
        REQUIRE_THAT(r.output, Catch::Matchers::ContainsSubstring("error:"));
        std::filesystem::remove(f);
    }
}

TEST_CASE("CLI: --syntax-fix option", "[cli][syntax]") {
    SECTION("valid document passes through") {
        auto f = tmp_path("fix_valid.tex");
        write_file(f, "\\begin{document}\nHello\n\\end{document}\n");
        auto r = run_cmd("--syntax-fix " + f);
        REQUIRE(r.exit_code == 0);
        REQUIRE_THAT(r.output, Catch::Matchers::ContainsSubstring("Hello"));
        std::filesystem::remove(f);
    }

    SECTION("unclosed brace is fixed") {
        auto f = tmp_path("fix_brace.tex");
        write_file(f, "\\textbf{Hello\n");
        auto r = run_cmd("--syntax-fix " + f);
        REQUIRE(r.exit_code == 0);
        REQUIRE_THAT(r.output, Catch::Matchers::ContainsSubstring("inserted '}'"));
        std::filesystem::remove(f);
    }

    SECTION("unclosed environment is fixed") {
        auto f = tmp_path("fix_env.tex");
        write_file(f, "\\begin{document}\nHello\n");
        auto r = run_cmd("--syntax-fix " + f);
        REQUIRE(r.exit_code == 0);
        REQUIRE_THAT(r.output, Catch::Matchers::ContainsSubstring("inserted '\\end{document}'"));
        std::filesystem::remove(f);
    }

    SECTION("unclosed math is fixed") {
        auto f = tmp_path("fix_math.tex");
        write_file(f, "$x+y\n");
        auto r = run_cmd("--syntax-fix " + f);
        REQUIRE(r.exit_code == 0);
        REQUIRE_THAT(r.output, Catch::Matchers::ContainsSubstring("inserted '$'"));
        std::filesystem::remove(f);
    }

    SECTION("without --syntax-fix, no fix is applied") {
        auto f = tmp_path("no_fix.tex");
        write_file(f, "\\textbf{Bold\n");
        auto r = run_cmd(f);
        REQUIRE(r.exit_code == 0);
        REQUIRE_THAT(r.output, !Catch::Matchers::ContainsSubstring("inserted"));
        std::filesystem::remove(f);
    }
}

TEST_CASE("CLI: --remove-tags", "[cli]") {
    std::string f = tmp_path("rmtag.tex");
    write_file(f, "\\begin{equation}\nE = mc^2 \\tag{1}\n\\end{equation}\n");

    SECTION("removes \\tag when flag is set") {
        auto r = run_cmd("--remove-tags " + f);
        REQUIRE(r.exit_code == 0);
        REQUIRE(r.output.find("\\tag") == std::string::npos);
        REQUIRE(r.output.find("E = mc^{2}") != std::string::npos);
    }

    SECTION("preserves \\tag without flag") {
        auto r = run_cmd(f);
        REQUIRE(r.exit_code == 0);
        REQUIRE(r.output.find("\\tag{1}") != std::string::npos);
    }

    std::filesystem::remove(f);
}

TEST_CASE("CLI: --display-math-style", "[cli]") {
    std::string f;
    {
        std::ofstream ofs("/tmp/.test-latex-fmt-dms.tex");
        ofs << "\\[\nx\n\\]\n$$y$$";
        f = "/tmp/.test-latex-fmt-dms.tex";
    }

    SECTION("default is dollar style") {
        auto r = run_cmd(f);
        REQUIRE(r.exit_code == 0);
        REQUIRE(r.output.find("$$") != std::string::npos);
    }

    SECTION("bracket style") {
        auto r = run_cmd("--display-math-style=bracket " + f);
        REQUIRE(r.exit_code == 0);
        REQUIRE(r.output.find("\\[") != std::string::npos);
        REQUIRE(r.output.find("\\]") != std::string::npos);
        REQUIRE(r.output.find("$$") == std::string::npos);
    }

    SECTION("equation style") {
        auto r = run_cmd("--display-math-style=equation " + f);
        REQUIRE(r.exit_code == 0);
        REQUIRE(r.output.find("\\begin{equation}") != std::string::npos);
        REQUIRE(r.output.find("\\end{equation}") != std::string::npos);
        REQUIRE(r.output.find("equation*") == std::string::npos);
    }

    SECTION("equation* style") {
        auto r = run_cmd("--display-math-style=equation* " + f);
        REQUIRE(r.exit_code == 0);
        REQUIRE(r.output.find("\\begin{equation*}") != std::string::npos);
        REQUIRE(r.output.find("\\end{equation*}") != std::string::npos);
    }

    SECTION("invalid style returns error") {
        auto r = run_cmd("--display-math-style=invalid " + f);
        REQUIRE(r.exit_code != 0);
    }

    SECTION("no-unify preserves original") {
        auto r = run_cmd("--no-math-unify --display-math-style=equation " + f);
        REQUIRE(r.exit_code == 0);
        REQUIRE(r.output.find("\\[") != std::string::npos);
        REQUIRE(r.output.find("$$") != std::string::npos);
        REQUIRE(r.output.find("\\begin{equation}") == std::string::npos);
    }

    std::filesystem::remove(f);
}
