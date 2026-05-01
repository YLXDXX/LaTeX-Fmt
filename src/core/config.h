#pragma once
#include <string>
#include <fstream>
#include <cstdlib>

namespace latex_fmt {

struct FormatConfig {
    int indent_width = 2;
    int max_line_width = 0;
    bool cjk_spacing = true;
    bool brace_completion = true;
    bool comment_normalize = true;
    bool blank_line_compress = true;
    bool trailing_whitespace_remove = true;
    bool display_math_format = true;
    bool math_delimiter_unify = true;
    bool wrap = false;
    bool wrap_paragraphs = false;

    bool load_from_file(const std::string& path) {
        std::ifstream ifs(path);
        if (!ifs) return false;

        std::string line;
        while (std::getline(ifs, line)) {
            size_t eq = line.find('=');
            if (eq == std::string::npos) continue;

            std::string key = trim(line.substr(0, eq));
            std::string value = trim(line.substr(eq + 1));

            if (key.empty() || key[0] == '#') continue;

            if (key == "indent_width") {
                indent_width = std::atoi(value.c_str());
            } else if (key == "max_line_width") {
                max_line_width = std::atoi(value.c_str());
            } else if (key == "cjk_spacing") {
                cjk_spacing = parse_bool(value, true);
            } else if (key == "brace_completion") {
                brace_completion = parse_bool(value, true);
            } else if (key == "comment_normalize") {
                comment_normalize = parse_bool(value, true);
            } else if (key == "blank_line_compress") {
                blank_line_compress = parse_bool(value, true);
            } else if (key == "trailing_whitespace_remove") {
                trailing_whitespace_remove = parse_bool(value, true);
            } else if (key == "display_math_format") {
                display_math_format = parse_bool(value, true);
            } else if (key == "math_delimiter_unify") {
                math_delimiter_unify = parse_bool(value, true);
            } else if (key == "wrap") {
                wrap = parse_bool(value, false);
            } else if (key == "wrap_paragraphs") {
                wrap_paragraphs = parse_bool(value, false);
            }
        }
        return true;
    }

private:
    static std::string trim(const std::string& s) {
        size_t start = s.find_first_not_of(" \t\r");
        if (start == std::string::npos) return "";
        size_t end = s.find_last_not_of(" \t\r");
        return s.substr(start, end - start + 1);
    }

    static bool parse_bool(const std::string& v, bool default_val) {
        if (v.empty()) return default_val;
        if (v == "true" || v == "1" || v == "yes" || v == "on") return true;
        if (v == "false" || v == "0" || v == "no" || v == "off") return false;
        return default_val;
    }
};

} // namespace latex_fmt
