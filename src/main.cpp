#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <cstring>
#include <cstdlib>
#include <vector>
#include <algorithm>
#include <filesystem>
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
        << "  -r, --recursive          Recursively process all .tex files in directory\n"
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
        << "  --wrap                   Wrap long lines at word boundaries\n"
        << "  --wrap-paragraphs        Wrap paragraph text lines\n"
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

static bool is_wrappable_line(const std::string& line, bool wrap_paragraphs) {
    if (line.empty()) return false;
    size_t first = line.find_first_not_of(" \t");
    if (first == std::string::npos) return false;
    char c = line[first];
    if (c == '%') return true;
    if (c == '\\') return false;
    if (c == '$') return false;
    if (!wrap_paragraphs) return false;
    if (line.find('$') != std::string::npos) return false;
    if (line.find("\\begin") != std::string::npos) return false;
    if (line.find("\\end") != std::string::npos) return false;
    return true;
}

static std::string wrap_line(const std::string& line, int max_width, const std::string& cont_indent) {
    if ((int)line.size() <= max_width) return line;

    std::string result;
    size_t pos = 0;
    bool first_part = true;

    while (pos < line.size()) {
        if (!first_part) result += "\n" + cont_indent;

        size_t avail = max_width;
        if (!first_part && (int)cont_indent.size() < avail) avail -= cont_indent.size();

        size_t remaining = line.size() - pos;
        if (remaining <= avail) {
            result += line.substr(pos);
            break;
        }

        size_t search_end = pos + avail;
        if (search_end > line.size()) search_end = line.size();

        size_t break_at = std::string::npos;

        for (int pass = 0; pass < 3 && break_at == std::string::npos; ++pass) {
            for (size_t i = search_end; i > pos; --i) {
                char ch = line[i - 1];
                bool found = false;
                if (pass == 0 && (ch == '.' || ch == '?' || ch == '!') && i < line.size()) found = true;
                if (pass == 1 && (ch == ';' || ch == ':' || ch == ',')) found = true;
                if (pass == 2 && ch == ' ') found = true;
                if (found) {
                    break_at = i;
                    break;
                }
            }
        }

        if (break_at == std::string::npos || break_at <= pos) {
            break_at = search_end;
        }

        result += line.substr(pos, break_at - pos);
        pos = break_at;
        while (pos < line.size() && line[pos] == ' ') ++pos;
        first_part = false;
    }

    return result;
}

static std::string apply_wrapping(const std::string& formatted, const latex_fmt::FormatConfig& config) {
    if (!config.wrap && !config.wrap_paragraphs) return formatted;
    if (config.max_line_width <= 0) return formatted;

    auto lines = split_lines(formatted);
    std::ostringstream out;

    for (const auto& line : lines) {
        if (!is_wrappable_line(line, config.wrap_paragraphs)) {
            out << line << "\n";
            continue;
        }

        if ((int)line.size() <= config.max_line_width) {
            out << line << "\n";
            continue;
        }

        size_t first = line.find_first_not_of(" \t");
        std::string orig_indent;
        std::string body;
        if (first == std::string::npos) {
            out << line << "\n";
            continue;
        }
        orig_indent = line.substr(0, first);
        body = line.substr(first);
        std::string cont = orig_indent + "  ";

        out << orig_indent << wrap_line(body, config.max_line_width - (int)orig_indent.size(), cont) << "\n";
    }

    return out.str();
}

int main(int argc, char* argv[]) {
    latex_fmt::FormatConfig config;
    bool in_place = false;
    bool check_only = false;
    bool diff_mode = false;
    bool quiet = false;
    bool recursive = false;
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

    auto format_string = [&](const std::string& src) -> std::pair<std::string, std::vector<std::string>> {
        latex_fmt::Registry reg;
        reg.registerBuiltin();
        latex_fmt::Lexer lex(src, reg);
        latex_fmt::Parser par(lex.tokenize(), src, reg);
        latex_fmt::FormatVisitor vis(reg, src, config);
        vis.visit(*par.parse());
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
            if (!read_input(f, input)) {
                changed++;
                continue;
            }
            if (input.empty()) { total++; continue; }

            auto [result, warnings] = format_string(input);
            if (config.wrap || config.wrap_paragraphs) {
                result = apply_wrapping(result, config);
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
                    std::cout << generate_unified_diff(input, result, "a/" + f, "b/" + f);
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
                    if (!write_output(f, result)) { changed++; continue; }
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
        if (!read_input(input_path, input)) return 1;
    } else {
        std::ostringstream ss;
        ss << std::cin.rdbuf();
        input = ss.str();
    }

    if (input.empty()) return 0;

    auto [result, warnings] = format_string(input);

    if (config.wrap || config.wrap_paragraphs) {
        result = apply_wrapping(result, config);
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
        std::string diff = generate_unified_diff(input, result, label_old, label_new);
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
        if (!write_output(input_path, result)) return 1;
    } else if (!output_path.empty()) {
        if (!write_output(output_path, result)) return 1;
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
