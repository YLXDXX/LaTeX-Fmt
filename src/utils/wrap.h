#pragma once
#include <sstream>
#include <string>
#include <vector>
#include "core/config.h"
#include "utils/diff.h"

namespace latex_fmt::utils {

inline bool is_wrappable_line(const std::string& line, bool wrap_paragraphs) {
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

inline std::string wrap_line(const std::string& line, int max_width, const std::string& cont_indent) {
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

inline std::string apply_wrapping(const std::string& formatted, const latex_fmt::FormatConfig& config) {
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

} // namespace latex_fmt::utils
