#include <iostream>
#include <string>
#include <sstream>
#include <cstdlib>
#include <vector>
#include <algorithm>
#include <filesystem>
#include "core/registry.h"
#include "core/config.h"
#include "core/syntax_check.h"
#include "core/syntax_fix.h"
#include "parse/lexer.h"
#include "parse/parser.h"
#include "format/visitor.h"
#include "utils/io.h"
#include "utils/diff.h"
#include "utils/wrap.h"
#include "md/md_converter.h"

static const char* LATEX_FMT_VERSION = "1.0.0";

static void print_help(const char* prog) {
    std::cerr
        << "latex-fmt — LaTeX code formatter (v" << LATEX_FMT_VERSION << ")\n"
        << "\n"
        << "Usage:\n"
        << "  " << prog << " [options] [file.tex]\n"
        << "  " << prog << " [options] < input.tex > output.tex\n"
        << "\n"
        << "Options:\n"
        << "  --help, -h               Show this help message and exit\n"
        << "  --version, -V            Show version and exit\n"
        << "  -i                       Format file in-place\n"
        << "  -o <file>                Write output to file\n"
        << "  -r, --recursive          Recursively process all .tex files in directory\n"
        << "  --check                  Check if file needs formatting (exit 1 if changes needed)\n"
        << "  --diff                   Show unified diff instead of formatted output\n"
        << "  --quiet, -q              Suppress warnings\n"
        << "  --syntax-check           Check syntax before formatting (exit 1 if errors found)\n"
        << "  --syntax-fix             Auto-fix syntax issues (unclosed braces, environments, etc.)\n"
        << "\n"
        << "Formatting options:\n"
        << "  --indent-width=N         Set indentation width (default: 2)\n"
        << "  --max-line-width=N       Warn when a line exceeds N chars (default: 0 = off)\n"
        << "  --no-cjk-spacing         Disable auto spacing between CJK and ASCII chars\n"
        << "  --no-brace-completion    Disable brace completion for commands like \\frac\n"
        << "  --no-comment-normalize   Disable comment normalization\n"
        << "  --no-blank-line-compress Disable consecutive blank line compression\n"
        << "  --keep-trailing-spaces   Preserve trailing whitespace\n"
        << "  --no-display-math-format Disable display math standalone line formatting\n"
        << "  --no-math-unify          Disable math delimiter unification\n"
        << "  --wrap                   Wrap long lines at word boundaries\n"
        << "  --wrap-paragraphs        Wrap paragraph text lines\n"
        << "  --md                     Treat input as Markdown, convert to LaTeX and format\n"
        << "  --config-file=<path>     Read config from file (default: .latexfmtrc)\n"
        << "\n"
        << "Examples:\n"
        << "  " << prog << " paper.tex                  Format to stdout\n"
        << "  " << prog << " -i paper.tex                Format file in-place\n"
        << "  " << prog << " -o out.tex paper.tex        Format to output file\n"
        << "  " << prog << " --indent-width=4 paper.tex  Use 4-space indentation\n"
        << "  " << prog << " < input.tex > output.tex    Pipe from stdin\n";
}

static void print_version() {
    std::cout << "latex-fmt v" << LATEX_FMT_VERSION << "\n";
}

int main(int argc, char* argv[]) {
    latex_fmt::FormatConfig config;
    bool in_place = false;
    bool check_only = false;
    bool diff_mode = false;
    bool quiet = false;
    bool recursive = false;
    bool md_mode = false;
    bool syntax_check = false;
    bool syntax_fix = false;
    std::string output_path;
    std::string input_path;
    std::string config_file_path;

    for (int i = 1; i < argc; ++i) {
        std::string_view arg(argv[i]);
        if (arg == "--help" || arg == "-h") {
            print_help(argv[0]);
            return 0;
        }
        if (arg == "--version" || arg == "-V") {
            print_version();
            return 0;
        }
        if (arg.compare(0, 14, "--config-file=") == 0) {
            config_file_path = std::string(arg.substr(14));
        }
    }

    if (!config_file_path.empty()) {
        if (!config.load_from_file(config_file_path)) {
            std::cerr << "latex-fmt: warning: could not read config file '"
                      << config_file_path << "'\n";
        }
    } else {
        if (const char* home = std::getenv("HOME")) {
            std::string home_rc = std::string(home) + "/.latexfmtrc";
            config.load_from_file(home_rc);
        }
        config.load_from_file(".latexfmtrc");
    }

    for (int i = 1; i < argc; ++i) {
        std::string_view arg(argv[i]);
        if (arg == "--help" || arg == "-h" || arg == "--version" || arg == "-V") {
            continue;
        }
        if (arg.compare(0, 14, "--config-file=") == 0) {
            continue;
        }
        if (arg == "-i") {
            in_place = true;
            continue;
        }
        if (arg == "-r" || arg == "--recursive") {
            recursive = true;
            continue;
        }
        if (arg == "-o") {
            if (i + 1 < argc) {
                output_path = argv[++i];
            } else {
                std::cerr << "latex-fmt: error: -o requires a file argument\n";
                return 1;
            }
            continue;
        }
        if (arg == "--check") {
            check_only = true;
            continue;
        }
        if (arg == "--diff") {
            diff_mode = true;
            continue;
        }
        if (arg == "--quiet" || arg == "-q") {
            quiet = true;
            continue;
        }
        if (arg.compare(0, 17, "--max-line-width=") == 0) {
            config.max_line_width = std::stoi(std::string(arg.substr(17)));
            continue;
        }
        if (arg.compare(0, 15, "--indent-width=") == 0) {
            config.indent_width = std::stoi(std::string(arg.substr(15)));
            continue;
        }
        if (arg == "--no-cjk-spacing") {
            config.cjk_spacing = false;
            continue;
        }
        if (arg == "--cjk-spacing") {
            config.cjk_spacing = true;
            continue;
        }
        if (arg == "--no-brace-completion") {
            config.brace_completion = false;
            continue;
        }
        if (arg == "--brace-completion") {
            config.brace_completion = true;
            continue;
        }
        if (arg == "--no-comment-normalize") {
            config.comment_normalize = false;
            continue;
        }
        if (arg == "--comment-normalize") {
            config.comment_normalize = true;
            continue;
        }
        if (arg == "--no-blank-line-compress") {
            config.blank_line_compress = false;
            continue;
        }
        if (arg == "--blank-line-compress") {
            config.blank_line_compress = true;
            continue;
        }
        if (arg == "--keep-trailing-spaces") {
            config.trailing_whitespace_remove = false;
            continue;
        }
        if (arg == "--remove-trailing-spaces") {
            config.trailing_whitespace_remove = true;
            continue;
        }
        if (arg == "--no-display-math-format") {
            config.display_math_format = false;
            continue;
        }
        if (arg == "--display-math-format") {
            config.display_math_format = true;
            continue;
        }
        if (arg == "--no-math-unify") {
            config.math_delimiter_unify = false;
            continue;
        }
        if (arg == "--math-unify") {
            config.math_delimiter_unify = true;
            continue;
        }
        if (arg == "--wrap") {
            config.wrap = true;
            continue;
        }
        if (arg == "--wrap-paragraphs") {
            config.wrap_paragraphs = true;
            continue;
        }
        if (arg == "--md") {
            md_mode = true;
            continue;
        }
        if (arg == "--syntax-check") {
            syntax_check = true;
            continue;
        }
        if (arg == "--syntax-fix") {
            syntax_fix = true;
            continue;
        }
        if (arg.size() > 0 && arg[0] == '-') {
            std::cerr << "latex-fmt: error: unknown option '" << arg << "'\n";
            print_help(argv[0]);
            return 1;
        }
        if (input_path.empty()) {
            input_path = arg;
        } else {
            std::cerr << "latex-fmt: error: multiple input files not supported\n";
            return 1;
        }
    }

    if (in_place && !output_path.empty()) {
        std::cerr << "latex-fmt: error: -i and -o cannot be used together\n";
        return 1;
    }

    if (check_only && (in_place || !output_path.empty())) {
        std::cerr << "latex-fmt: error: --check cannot be used with -i or -o\n";
        return 1;
    }

    if (diff_mode && (in_place || !output_path.empty())) {
        std::cerr << "latex-fmt: error: --diff cannot be used with -i or -o\n";
        return 1;
    }

    if (recursive && !input_path.empty() && !output_path.empty()) {
        std::cerr << "latex-fmt: error: -r cannot be used with -o (output to directory not supported)\n";
        return 1;
    }

    auto format_string = [&](const std::string& src, bool do_syntax_check = false,
                              bool do_syntax_fix = false)
        -> std::pair<std::string, std::vector<std::string>> {
        std::string source = src;
        if (md_mode) {
            source = latex_fmt::MdConverter().convert(src);
        }
        latex_fmt::Registry reg;
        reg.registerBuiltin();
        latex_fmt::Lexer lex(source, reg);
        latex_fmt::Parser par(lex.tokenize(), source, reg);
        auto doc = par.parse();

        if (do_syntax_check || do_syntax_fix) {
            latex_fmt::SyntaxChecker checker(source, *doc);
            auto errors = checker.check();

            if (!errors.empty()) {
                if (do_syntax_fix) {
                    latex_fmt::SyntaxFixer fixer(source, *doc);
                    source = fixer.apply();

                    for (const auto& fix : fixer.getFixes()) {
                        auto [line, col] = checker.getLineCol(fix.offset);
                        std::cerr << "line " << line << ":" << col
                                  << ": fixed: inserted '" << fix.text << "'\n";
                    }

                    latex_fmt::Lexer lex2(source, reg);
                    auto tokens2 = lex2.tokenize();
                    latex_fmt::Parser par2(std::move(tokens2), source, reg);
                    doc = par2.parse();
                } else {
                    for (const auto& e : errors) {
                        std::cerr << "line " << e.line << ":" << e.col
                                  << ": error: " << e.message << "\n";
                    }
                    return {"", {}};
                }
            }
        }

        latex_fmt::FormatVisitor vis(reg, source, config);
        vis.visit(*doc);
        return {vis.extractOutput(), vis.getWarnings()};
    };

    if (recursive) {
        std::string dir = input_path.empty() ? "." : input_path;
        if (!std::filesystem::is_directory(dir)) {
            std::cerr << "latex-fmt: error: '" << dir << "' is not a directory\n";
            return 1;
        }

        std::vector<std::string> tex_files;
        for (auto& entry : std::filesystem::recursive_directory_iterator(dir)) {
            if (entry.is_regular_file() && entry.path().extension() == ".tex") {
                tex_files.push_back(entry.path().string());
            }
        }
        std::sort(tex_files.begin(), tex_files.end());

        if (tex_files.empty()) {
            std::cerr << "latex-fmt: no .tex files found in '" << dir << "'\n";
            return 0;
        }

        int total = 0, changed = 0;
        for (const auto& f : tex_files) {
            std::string input;
            if (!latex_fmt::utils::read_input(f, input)) {
                changed++;
                continue;
            }
            if (input.empty()) { total++; continue; }

            auto [result, warnings] = format_string(input, syntax_check, syntax_fix);
            if (syntax_check && result.empty() && warnings.empty()) {
                std::cerr << f << ": syntax errors found\n";
                changed++;
                total++;
                continue;
            }
            if (syntax_fix && result.empty() && warnings.empty()) {
                std::cerr << f << ": syntax fix failed\n";
                changed++;
                total++;
                continue;
            }
            if (config.wrap || config.wrap_paragraphs) {
                result = latex_fmt::utils::apply_wrapping(result, config);
            }
            total++;

            bool was_changed = (input != result);

            if (check_only) {
                if (!quiet) {
                    for (const auto& w : warnings) {
                        std::cerr << f << ": WARNING: " << w << "\n";
                    }
                }
                if (was_changed) {
                    std::cerr << f << ": needs formatting\n";
                    changed++;
                }
                continue;
            }

            if (diff_mode) {
                if (was_changed) {
                    changed++;
                    std::cout << "=== " << f << " ===\n";
                    std::cout << latex_fmt::utils::generate_unified_diff(input, result, "a/" + f, "b/" + f);
                }
                if (!quiet) {
                    for (const auto& w : warnings) {
                        std::cerr << f << ": WARNING: " << w << "\n";
                    }
                }
                continue;
            }

            if (!quiet) {
                for (const auto& w : warnings) {
                    std::cerr << f << ": WARNING: " << w << "\n";
                }
            }

            if (in_place) {
                if (was_changed) {
                    if (!latex_fmt::utils::write_output(f, result)) { changed++; continue; }
                    changed++;
                }
            } else {
                if (was_changed) changed++;
                std::cout << "=== " << f << " ===\n" << result;
            }
        }

        if (check_only) {
            if (changed > 0) {
                std::cerr << "\n" << changed << " of " << total << " file(s) need formatting\n";
                return 1;
            }
            return 0;
        }

        if (in_place) {
            if (!quiet) std::cerr << total << " file(s) formatted, " << changed << " file(s) changed\n";
            return changed > 0 ? 0 : 0;
        }

        if (diff_mode) {
            return changed > 0 ? 1 : 0;
        }

        return 0;
    }

    std::string input;
    if (!input_path.empty()) {
        if (!latex_fmt::utils::read_input(input_path, input)) return 1;
    } else {
        std::ostringstream ss;
        ss << std::cin.rdbuf();
        input = ss.str();
    }

    if (input.empty()) return 0;

    auto [result, warnings] = format_string(input, syntax_check, syntax_fix);

    if (syntax_check && result.empty() && warnings.empty()) {
        return 1;
    }
    if (syntax_fix && result.empty() && warnings.empty()) {
        return 1;
    }

    if (config.wrap || config.wrap_paragraphs) {
        result = latex_fmt::utils::apply_wrapping(result, config);
    }

    if (check_only) {
        bool needs_fmt = (input != result);
        if (!quiet) {
            for (const auto& w : warnings) {
                std::cerr << "WARNING: " << w << "\n";
            }
        }
        if (needs_fmt) {
            if (!input_path.empty()) {
                std::cerr << input_path << ": needs formatting\n";
            } else {
                std::cerr << "<stdin>: needs formatting\n";
            }
            return 1;
        }
        return 0;
    }

    if (diff_mode) {
        std::string label_old = input_path.empty() ? "a/<stdin>" : "a/" + input_path;
        std::string label_new = input_path.empty() ? "b/<stdin>" : "b/" + input_path;
        std::string diff = latex_fmt::utils::generate_unified_diff(input, result, label_old, label_new);
        if (!diff.empty()) {
            std::cout << diff;
        }
        if (!quiet) {
            for (const auto& w : warnings) {
                std::cerr << "WARNING: " << w << "\n";
            }
        }
        return diff.empty() ? 0 : 1;
    }

    if (in_place) {
        if (!latex_fmt::utils::write_output(input_path, result)) return 1;
    } else if (!output_path.empty()) {
        if (!latex_fmt::utils::write_output(output_path, result)) return 1;
    } else {
        std::cout << result;
    }

    if (!quiet) {
        for (const auto& w : warnings) {
            std::cerr << "WARNING: " << w << "\n";
        }
    }

    return 0;
}
