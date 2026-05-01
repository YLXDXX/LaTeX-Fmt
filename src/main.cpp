#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <cstring>
#include <vector>
#include <algorithm>
#include "core/registry.h"
#include "core/config.h"
#include "parse/lexer.h"
#include "parse/parser.h"
#include "format/visitor.h"

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
        << "  --check                  Check if file needs formatting (exit 1 if changes needed)\n"
        << "  --diff                   Show unified diff instead of formatted output\n"
        << "  --quiet, -q              Suppress warnings\n"
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

static bool read_input(const std::string& path, std::string& out) {
    std::ifstream ifs(path);
    if (!ifs) {
        std::cerr << "latex-fmt: error: cannot read file '" << path << "'\n";
        return false;
    }
    std::ostringstream ss;
    ss << ifs.rdbuf();
    out = ss.str();
    return true;
}

static bool write_output(const std::string& path, const std::string& content) {
    std::ofstream ofs(path);
    if (!ofs) {
        std::cerr << "latex-fmt: error: cannot write file '" << path << "'\n";
        return false;
    }
    ofs << content;
    return true;
}

static std::vector<std::string> split_lines(const std::string& s) {
    std::vector<std::string> lines;
    size_t start = 0;
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '\n') {
            lines.push_back(s.substr(start, i - start));
            start = i + 1;
        }
    }
    if (start < s.size() || s.empty() || s.back() == '\n') {
        lines.push_back(s.substr(start));
    }
    return lines;
}

static std::vector<std::vector<int>> compute_lcs_table(
    const std::vector<std::string>& a, const std::vector<std::string>& b)
{
    int n = (int)a.size(), m = (int)b.size();
    std::vector<std::vector<int>> dp(n + 1, std::vector<int>(m + 1, 0));
    for (int i = 1; i <= n; ++i) {
        for (int j = 1; j <= m; ++j) {
            if (a[i - 1] == b[j - 1]) {
                dp[i][j] = dp[i - 1][j - 1] + 1;
            } else {
                dp[i][j] = std::max(dp[i - 1][j], dp[i][j - 1]);
            }
        }
    }
    return dp;
}

static std::string generate_unified_diff(
    const std::string& old_str, const std::string& new_str,
    const std::string& label_old, const std::string& label_new,
    int context = 3)
{
    auto old_lines = split_lines(old_str);
    auto new_lines = split_lines(new_str);

    if (old_lines == new_lines) return "";

    int n = (int)old_lines.size(), m = (int)new_lines.size();

    auto dp = compute_lcs_table(old_lines, new_lines);

    std::vector<bool> old_match(n, false), new_match(m, false);
    {
        int i = n, j = m;
        while (i > 0 && j > 0) {
            if (old_lines[i - 1] == new_lines[j - 1]) {
                old_match[i - 1] = true;
                new_match[j - 1] = true;
                --i; --j;
            } else if (dp[i - 1][j] >= dp[i][j - 1]) {
                --i;
            } else {
                --j;
            }
        }
    }

    struct Hunk {
        int old_start, old_count;
        int new_start, new_count;
    };

    std::vector<Hunk> hunks;
    int o = 0, ne = 0;
    while (o < n || ne < m) {
        while (o < n && old_match[o]) ++o;
        while (ne < m && new_match[ne]) ++ne;
        if (o >= n && ne >= m) break;

        int os = o, ns = ne;
        while (o < n && !old_match[o]) ++o;
        while (ne < m && !new_match[ne]) ++ne;

        int oc = o - os, nc = ne - ns;
        if (oc > 0 || nc > 0) {
            hunks.push_back({os, oc, ns, nc});
        }
    }

    std::ostringstream out;
    out << "--- " << label_old << "\n"
        << "+++ " << label_new << "\n";

    for (auto& h : hunks) {
        int ctx_os = std::max(0, h.old_start - context);
        int ctx_oe = std::min(n, h.old_start + h.old_count + context);
        int ctx_ns = std::max(0, h.new_start - context);
        int ctx_ne = std::min(m, h.new_start + h.new_count + context);

        out << "@@ -" << (ctx_os + 1) << "," << (ctx_oe - ctx_os)
            << " +" << (ctx_ns + 1) << "," << (ctx_ne - ctx_ns) << " @@\n";

        for (int k = ctx_os; k < h.old_start; ++k) {
            out << " " << old_lines[k] << "\n";
        }
        for (int k = 0; k < h.old_count; ++k) {
            out << "-" << old_lines[h.old_start + k] << "\n";
        }
        for (int k = 0; k < h.new_count; ++k) {
            out << "+" << new_lines[h.new_start + k] << "\n";
        }
        for (int k = h.old_start + h.old_count; k < ctx_oe; ++k) {
            out << " " << old_lines[k] << "\n";
        }
    }

    return out.str();
}

int main(int argc, char* argv[]) {
    latex_fmt::FormatConfig config;
    bool in_place = false;
    bool check_only = false;
    bool diff_mode = false;
    bool quiet = false;
    std::string output_path;
    std::string input_path;

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
        if (arg == "-i") {
            in_place = true;
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
        if (arg == "--no-brace-completion") {
            config.brace_completion = false;
            continue;
        }
        if (arg == "--no-comment-normalize") {
            config.comment_normalize = false;
            continue;
        }
        if (arg == "--no-blank-line-compress") {
            config.blank_line_compress = false;
            continue;
        }
        if (arg == "--keep-trailing-spaces") {
            config.trailing_whitespace_remove = false;
            continue;
        }
        if (arg == "--no-display-math-format") {
            config.display_math_format = false;
            continue;
        }
        if (arg == "--no-math-unify") {
            config.math_delimiter_unify = false;
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

    std::string input;
    if (!input_path.empty()) {
        if (!read_input(input_path, input)) return 1;
    } else {
        std::ostringstream ss;
        ss << std::cin.rdbuf();
        input = ss.str();
    }

    if (input.empty()) return 0;

    latex_fmt::Registry registry;
    registry.registerBuiltin();

    latex_fmt::Lexer lexer(input, registry);
    auto tokens = lexer.tokenize();

    latex_fmt::Parser parser(std::move(tokens), input, registry);
    auto ast = parser.parse();

    latex_fmt::FormatVisitor visitor(registry, input, config);
    visitor.visit(*ast);

    std::string result = visitor.extractOutput();

    if (check_only) {
        bool needs_fmt = (input != result);
        if (!quiet) {
            for (const auto& w : visitor.getWarnings()) {
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
        std::string diff = generate_unified_diff(input, result, label_old, label_new);
        if (!diff.empty()) {
            std::cout << diff;
        }
        if (!quiet) {
            for (const auto& w : visitor.getWarnings()) {
                std::cerr << "WARNING: " << w << "\n";
            }
        }
        return diff.empty() ? 0 : 1;
    }

    if (in_place) {
        if (!write_output(input_path, result)) return 1;
    } else if (!output_path.empty()) {
        if (!write_output(output_path, result)) return 1;
    } else {
        std::cout << result;
    }

    if (!quiet) {
        for (const auto& w : visitor.getWarnings()) {
            std::cerr << "WARNING: " << w << "\n";
        }
    }

    return 0;
}
