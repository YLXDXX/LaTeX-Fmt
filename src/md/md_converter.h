#pragma once
#include <algorithm>
#include <cctype>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace latex_fmt {

class MdConverter {
public:
    explicit MdConverter() {}

    std::string convert(std::string_view input);

private:
    // ═══════════════════════════════════════════
    //  Block-level state
    // ═══════════════════════════════════════════
    enum BlockState { Normal, InQuote, InList };

    BlockState block_state_ = Normal;
    int list_indent_ = 0;
    int list_type_ = 0;
    bool in_code_block_ = false;
    std::string code_fence_info_;

    // ═══════════════════════════════════════════
    //  Block-level helpers
    // ═══════════════════════════════════════════
    bool isBlankLine(const std::string& line) const;
    bool isCodeFence(const std::string& line) const;
    int detectHeading(const std::string& line, std::string& content) const;
    bool detectListItem(const std::string& line, std::string& marker,
                        std::string& content, int& indent) const;
    bool detectBlockquote(const std::string& line, std::string& content) const;

    std::string convertHeading(int level, const std::string& content);
    std::string convertBlockquote(std::vector<std::string>& lines);
    std::string convertList(std::vector<std::string>& items,
                            std::vector<int>& types, int indent);

    // ═══════════════════════════════════════════
    //  Inline processing pipeline
    // ═══════════════════════════════════════════
    std::string convertInline(std::string text);

    // Each step is a standalone method — add new steps here to extend
    struct CodeSpan { std::string content; };

    std::string extractCodeSpans(std::string text, std::vector<CodeSpan>& out);
    std::string convertEscapedChars(std::string text);
    std::string escapeLatexSpecials(std::string text);
    std::string convertLinks(std::string text);
    std::string convertEmphasis(std::string text);
    std::string protectLatexCommands(std::string text, std::vector<std::string>& out);
    std::string restoreLatexCommands(std::string text, const std::vector<std::string>& cmds);
    std::string restoreCodeSpans(std::string text, const std::vector<CodeSpan>& codes);

    // ═══════════════════════════════════════════
    //  Utility
    // ═══════════════════════════════════════════
    static std::string ltrim(std::string s) {
        size_t p = s.find_first_not_of(" \t");
        if (p == std::string::npos) return "";
        return s.substr(p);
    }
};

// ───────────────────────────────────────────────
//  Public entry point
// ───────────────────────────────────────────────

inline std::string MdConverter::convert(std::string_view input) {
    std::vector<std::string> lines;
    std::string line_buf;

    for (size_t i = 0; i < input.size(); ++i) {
        if (input[i] == '\n') {
            if (!line_buf.empty() && line_buf.back() == '\r') line_buf.pop_back();
            lines.push_back(std::move(line_buf));
            line_buf.clear();
        } else {
            line_buf += input[i];
        }
    }
    if (!line_buf.empty() || (input.empty() && !lines.empty()) || 
        (!input.empty() && input.back() == '\n')) {
        if (!line_buf.empty() && line_buf.back() == '\r') line_buf.pop_back();
        lines.push_back(std::move(line_buf));
    }

    std::ostringstream out;
    std::vector<std::string> block_lines;
    std::vector<std::string> list_items;
    std::vector<int> list_types;
    std::vector<std::string> quote_lines;
    bool list_open = false;

    auto flushBlock = [&]() {
        if (list_open) {
            out << convertList(list_items, list_types, list_indent_);
            list_items.clear();
            list_types.clear();
            list_open = false;
            list_type_ = 0;
            list_indent_ = 0;
        } else if (!quote_lines.empty()) {
            out << convertBlockquote(quote_lines);
            quote_lines.clear();
        } else if (!block_lines.empty()) {
            out << convertInline(block_lines[0]);
            for (size_t i = 1; i < block_lines.size(); ++i) {
                out << " " << convertInline(block_lines[i]);
            }
            out << "\n";
            block_lines.clear();
        }
    };

    auto beginList = [&](const std::string& marker, int type, int indent) {
        if (!list_open) {
            flushBlock();
        }
        list_open = true;
        list_type_ = type;
        list_indent_ = indent;
        block_state_ = InList;
    };

    auto maybeEndList = [&](int indent) {
        if (list_open && indent < list_indent_) {
            out << convertList(list_items, list_types, list_indent_);
            list_items.clear();
            list_types.clear();
            list_open = false;
            list_type_ = 0;
            block_state_ = Normal;
        }
    };

    for (const auto& raw_line : lines) {
        std::string line = raw_line;

        if (in_code_block_) {
            if (isCodeFence(line)) {
                in_code_block_ = false;
                out << "\\end{verbatim}\n";
                continue;
            }
            out << raw_line << "\n";
            continue;
        }

        if (isCodeFence(line)) {
            flushBlock();
            in_code_block_ = true;
            code_fence_info_.clear();
            if (line.size() > 3) {
                code_fence_info_ = ltrim(line.substr(3));
            }
            out << "\\begin{verbatim}\n";
            continue;
        }

        if (isBlankLine(line)) {
            flushBlock();
            out << "\n";
            continue;
        }

        std::string heading_content;
        int h_level = detectHeading(line, heading_content);
        if (h_level > 0) {
            flushBlock();
            out << convertHeading(h_level, heading_content) << "\n";
            continue;
        }

        std::string list_marker, list_content;
        int item_indent = 0;
        if (detectListItem(line, list_marker, list_content, item_indent)) {
            int type = (list_marker[0] >= '0' && list_marker[0] <= '9') ? 2 : 1;
            maybeEndList(item_indent);

            if (!list_open) {
                beginList(list_marker, type, item_indent);
            }
            list_items.push_back(list_content);
            list_types.push_back(type);
            continue;
        }

        std::string quote_content;
        if (detectBlockquote(line, quote_content)) {
            if (!quote_lines.empty() || block_lines.empty()) {
                if (!block_lines.empty()) {
                    quote_lines.insert(quote_lines.end(),
                                       block_lines.begin(), block_lines.end());
                    block_lines.clear();
                }
            } else {
                flushBlock();
            }
            quote_lines.push_back(quote_content);
            block_state_ = InQuote;
            continue;
        }

        flushBlock();
        block_lines.push_back(line);
        block_state_ = Normal;
    }

    flushBlock();
    return out.str();
}

// ───────────────────────────────────────────────
//  Block-level detection helpers
// ───────────────────────────────────────────────

inline bool MdConverter::isBlankLine(const std::string& line) const {
    return line.find_first_not_of(" \t") == std::string::npos;
}

inline bool MdConverter::isCodeFence(const std::string& line) const {
    if (line.size() >= 3 && line[0] == '`' && line[1] == '`' && line[2] == '`')
        return true;
    return false;
}

inline int MdConverter::detectHeading(const std::string& line,
                                        std::string& content) const {
    size_t pos = 0;
    while (pos < line.size() && line[pos] == '#') ++pos;
    if (pos == 0 || pos > 5) return 0;
    if (pos < line.size() && line[pos] != ' ') return 0;
    content = ltrim(line.substr(pos));
    return static_cast<int>(pos);
}

inline bool MdConverter::detectListItem(const std::string& line,
                                         std::string& marker,
                                         std::string& content,
                                         int& indent) const {
    indent = 0;
    while (indent < (int)line.size() && line[indent] == ' ') ++indent;

    size_t rest = indent;
    if (rest >= line.size()) return false;

    char c = line[rest];
    if (c == '-' || c == '+' || c == '*') {
        if (rest + 1 < line.size() && line[rest + 1] == ' ') {
            marker = std::string(1, c);
            content = ltrim(line.substr(rest + 2));
            return true;
        }
    }

    if (c >= '0' && c <= '9') {
        size_t p = rest;
        while (p < line.size() && line[p] >= '0' && line[p] <= '9') ++p;
        if (p < line.size() && line[p] == '.' && p + 1 < line.size() && line[p + 1] == ' ') {
            marker = line.substr(rest, p - rest + 1);
            content = ltrim(line.substr(p + 2));
            return true;
        }
    }

    return false;
}

inline bool MdConverter::detectBlockquote(const std::string& line,
                                           std::string& content) const {
    if (!line.empty() && line[0] == '>') {
        if (line.size() > 1 && line[1] == ' ') {
            content = line.substr(2);
        } else {
            content = line.substr(1);
        }
        return true;
    }
    return false;
}

// ───────────────────────────────────────────────
//  Block-level conversion
// ───────────────────────────────────────────────

inline std::string MdConverter::convertHeading(int level,
                                                const std::string& content) {
    static const char* cmds[] = {
        "", "section", "subsection", "subsubsection", "paragraph", "subparagraph"
    };
    if (level < 1 || level > 5) return convertInline(content) + "\n";
    return "\\" + std::string(cmds[level]) + "{" + convertInline(content) + "}";
}

inline std::string MdConverter::convertBlockquote(std::vector<std::string>& lines) {
    std::ostringstream out;
    out << "\\begin{quote}\n";
    for (const auto& l : lines) {
        out << convertInline(l) << "\n";
    }
    out << "\\end{quote}\n";
    return out.str();
}

inline std::string MdConverter::convertList(std::vector<std::string>& items,
                                              std::vector<int>& types,
                                              int indent) {
    if (items.empty()) return "";
    std::ostringstream out;
    std::string prefix(indent, ' ');
    int prev_type = 0;

    for (size_t i = 0; i < items.size(); ++i) {
        if (types[i] != prev_type) {
            if (prev_type != 0) {
                out << prefix << (prev_type == 1 ? "\\end{itemize}\n"
                                                  : "\\end{enumerate}\n");
            }
            out << prefix << (types[i] == 1 ? "\\begin{itemize}\n"
                                             : "\\begin{enumerate}\n");
            prev_type = types[i];
        }
        out << prefix << "  \\item " << convertInline(items[i]) << "\n";
    }
    if (prev_type != 0) {
        out << prefix << (prev_type == 1 ? "\\end{itemize}\n"
                                          : "\\end{enumerate}\n");
    }
    return out.str();
}

// ───────────────────────────────────────────────
//  Inline processing pipeline
// ───────────────────────────────────────────────

inline std::string MdConverter::convertInline(std::string text) {
    std::vector<CodeSpan> codes;
    text = extractCodeSpans(std::move(text), codes);
    text = convertEscapedChars(std::move(text));
    text = convertLinks(std::move(text));
    text = convertEmphasis(std::move(text));

    std::vector<std::string> latex_cmds;
    text = protectLatexCommands(std::move(text), latex_cmds);
    text = escapeLatexSpecials(std::move(text));
    text = restoreLatexCommands(std::move(text), latex_cmds);

    text = restoreCodeSpans(std::move(text), codes);
    return text;
}

// ───────────────────────────────────────────────
//  Step 1: Extract inline code spans
// ───────────────────────────────────────────────

inline std::string MdConverter::extractCodeSpans(std::string text,
                                                  std::vector<CodeSpan>& out) {
    std::string result;
    size_t pos = 0;
    size_t marker_idx = 0;

    while (pos < text.size()) {
        if (text[pos] == '`') {
            size_t start = pos + 1;
            while (start < text.size() && text[start] == '`') ++start;
            if (start > pos + 1) {
                result += text.substr(pos, start - pos);
                pos = start;
                continue;
            }
            size_t end = start;
            while (end < text.size() && text[end] != '`') ++end;
            if (end < text.size()) {
                std::string code = text.substr(start, end - start);
                result += '\x01';
                result += std::to_string(marker_idx);
                result += '\x01';
                out.push_back({code});
                ++marker_idx;
                pos = end + 1;
                continue;
            }
        }
        result += text[pos];
        ++pos;
    }
    return result;
}

// ───────────────────────────────────────────────
//  Step 2: Convert Markdown escape sequences
//  \* → literal char (already LaTeX-safe for *)
//  \_ → literal char (will be escaped later)
// ───────────────────────────────────────────────

inline std::string MdConverter::convertEscapedChars(std::string text) {
    static const char* specials = "*_`[]&%$#{}~^\\";
    std::string result;

    for (size_t i = 0; i < text.size(); ++i) {
        if (text[i] == '\\' && i + 1 < text.size()) {
            char next = text[i + 1];
            bool is_md_special = false;
            for (const char* p = specials; *p; ++p) {
                if (next == *p) { is_md_special = true; break; }
            }
            if (is_md_special) {
                if (next == '\\') {
                    result += "\x02" "bs" "\x02";
                } else if (next == '_') {
                    result += "\x02" "us" "\x02";
                } else if (next == '{') {
                    result += "\x02" "ob" "\x02";
                } else if (next == '}') {
                    result += "\x02" "cb" "\x02";
                } else if (next == '&') {
                    result += "\x02" "am" "\x02";
                } else if (next == '%') {
                    result += "\x02" "pc" "\x02";
                } else if (next == '$') {
                    result += "\x02" "dl" "\x02";
                } else if (next == '#') {
                    result += "\x02" "hs" "\x02";
                } else if (next == '~') {
                    result += "\x02" "td" "\x02";
                } else if (next == '^') {
                    result += "\x02" "ct" "\x02";
                } else {
                    result += next;
                }
                ++i;
                continue;
            }
        }
        result += text[i];
    }
    return result;
}

// ───────────────────────────────────────────────
//  Step 3: Escape remaining LaTeX special chars
// ───────────────────────────────────────────────

inline std::string MdConverter::escapeLatexSpecials(std::string text) {
    std::string result;
    for (size_t i = 0; i < text.size(); ++i) {
        char c = text[i];
        if (c == '\\' && i + 1 < text.size() && std::isalpha(static_cast<unsigned char>(text[i + 1]))) {
            result += text[i];
            continue;
        }
        switch (c) {
            case '\\': result += "\\textbackslash{}"; break;
            case '{':  result += "\\{"; break;
            case '}':  result += "\\}"; break;
            case '&':  result += "\\&"; break;
            case '%':  result += "\\%"; break;
            case '$':  result += "\\$"; break;
            case '#':  result += "\\#"; break;
            case '_':  result += "\\_"; break;
            case '~':  result += "\\textasciitilde{}"; break;
            case '^':  result += "\\textasciicircum{}"; break;
            default:   result += c;
        }
    }

    std::string final_result;
    for (size_t i = 0; i < result.size(); ++i) {
        if (result[i] == '\x02') {
            size_t end = result.find('\x02', i + 1);
            if (end != std::string::npos) {
                std::string marker = result.substr(i + 1, end - i - 1);
                if (marker == "bs") final_result += "\\textbackslash{}";
                else if (marker == "us") final_result += "\\_";
                else if (marker == "ob") final_result += "\\{";
                else if (marker == "cb") final_result += "\\}";
                else if (marker == "am") final_result += "\\&";
                else if (marker == "pc") final_result += "\\%";
                else if (marker == "dl") final_result += "\\$";
                else if (marker == "hs") final_result += "\\#";
                else if (marker == "td") final_result += "\\textasciitilde{}";
                else if (marker == "ct") final_result += "\\textasciicircum{}";
                i = end;
                continue;
            }
        }
        final_result += result[i];
    }
    return final_result;
}

// ───────────────────────────────────────────────
//  Step 4: Convert links  [text](url)
// ───────────────────────────────────────────────

inline std::string MdConverter::convertLinks(std::string text) {
    std::string result;
    size_t pos = 0;

    while (pos < text.size()) {
        size_t bracket = text.find('[', pos);
        if (bracket == std::string::npos) {
            result += text.substr(pos);
            break;
        }
        result += text.substr(pos, bracket - pos);

        size_t close_bracket = text.find(']', bracket + 1);
        if (close_bracket == std::string::npos) {
            result += text[bracket];
            pos = bracket + 1;
            continue;
        }

        if (close_bracket + 1 >= text.size() || text[close_bracket + 1] != '(') {
            result += text[bracket];
            pos = bracket + 1;
            continue;
        }

        size_t close_paren = text.find(')', close_bracket + 2);
        if (close_paren == std::string::npos) {
            result += text[bracket];
            pos = bracket + 1;
            continue;
        }

        std::string link_text = text.substr(bracket + 1, close_bracket - bracket - 1);
        std::string url = text.substr(close_bracket + 2, close_paren - close_bracket - 2);
        result += "\\href{" + url + "}{" + convertInline(link_text) + "}";
        pos = close_paren + 1;
    }
    return result;
}

// ───────────────────────────────────────────────
//  Step 5: Convert emphasis  *** / ** / *  and  _ variants
// ───────────────────────────────────────────────

inline std::string MdConverter::convertEmphasis(std::string text) {
    for (int pass = 0; pass < 2; ++pass) {
        const char delim = (pass == 0) ? '*' : '_';
        bool changed = true;
        while (changed) {
            changed = false;
            std::string result;
            size_t pos = 0;
            while (pos < text.size()) {
                if (text[pos] == delim && pos + 2 < text.size() &&
                    text[pos + 1] == delim && text[pos + 2] == delim) {
                    size_t end = text.find(std::string(3, delim), pos + 3);
                    if (end != std::string::npos) {
                        std::string inner = text.substr(pos + 3, end - pos - 3);
                        result += "\\textbf{\\textit{" + inner + "}}";
                        pos = end + 3;
                        changed = true;
                        continue;
                    }
                }
                if (text[pos] == delim && pos + 1 < text.size() &&
                    text[pos + 1] == delim) {
                    size_t end = text.find(std::string(2, delim), pos + 2);
                    if (end != std::string::npos) {
                        std::string inner = text.substr(pos + 2, end - pos - 2);
                        result += "\\textbf{" + inner + "}";
                        pos = end + 2;
                        changed = true;
                        continue;
                    }
                }
                if (text[pos] == delim) {
                    size_t end = text.find(delim, pos + 1);
                    if (end != std::string::npos) {
                        std::string inner = text.substr(pos + 1, end - pos - 1);
                        result += "\\textit{" + inner + "}";
                        pos = end + 1;
                        changed = true;
                        continue;
                    }
                }
                result += text[pos];
                ++pos;
            }
            text = std::move(result);
        }
    }
    return text;
}

// ───────────────────────────────────────────────
//  Step 6: Protect generated LaTeX commands from escaping
// ───────────────────────────────────────────────

inline std::string MdConverter::protectLatexCommands(std::string text,
                                                      std::vector<std::string>& out) {
    std::string result;
    size_t pos = 0;

    while (pos < text.size()) {
        if (text[pos] == '\\' && pos + 1 < text.size() &&
            std::isalpha(static_cast<unsigned char>(text[pos + 1]))) {
            size_t cmd_start = pos;
            pos++;
            while (pos < text.size() && std::isalpha(static_cast<unsigned char>(text[pos]))) ++pos;

            size_t total_end = pos;
            while (total_end < text.size() && text[total_end] == '{') {
                int depth = 1;
                size_t bpos = total_end + 1;
                while (bpos < text.size() && depth > 0) {
                    if (text[bpos] == '{') ++depth;
                    else if (text[bpos] == '}') --depth;
                    ++bpos;
                }
                if (depth == 0) total_end = bpos;
                else break;
            }

            if (total_end > cmd_start) {
                std::string cmd = text.substr(cmd_start, total_end - cmd_start);
                result += "\x03" + std::to_string(out.size()) + "\x03";
                out.push_back(cmd);
                pos = total_end;
                continue;
            }
        }
        result += text[pos];
        ++pos;
    }
    return result;
}

inline std::string MdConverter::restoreLatexCommands(std::string text,
                                                      const std::vector<std::string>& cmds) {
    std::string result;
    size_t pos = 0;
    while (pos < text.size()) {
        if (text[pos] == '\x03') {
            size_t end = text.find('\x03', pos + 1);
            if (end != std::string::npos) {
                std::string idx_str = text.substr(pos + 1, end - pos - 1);
                int idx = std::stoi(idx_str);
                result += cmds[idx];
                pos = end + 1;
                continue;
            }
        }
        result += text[pos];
        ++pos;
    }
    return result;
}

// ───────────────────────────────────────────────
//  Step 7: Restore code spans as \texttt{...}
// ───────────────────────────────────────────────

inline std::string MdConverter::restoreCodeSpans(std::string text,
                                                   const std::vector<CodeSpan>& codes) {
    std::string result;
    size_t pos = 0;
    while (pos < text.size()) {
        if (text[pos] == '\x01') {
            size_t end = text.find('\x01', pos + 1);
            if (end != std::string::npos) {
                std::string idx_str = text.substr(pos + 1, end - pos - 1);
                int idx = std::stoi(idx_str);
                result += "\\texttt{" + codes[idx].content + "}";
                pos = end + 1;
                continue;
            }
        }
        result += text[pos];
        ++pos;
    }
    return result;
}

} // namespace latex_fmt
