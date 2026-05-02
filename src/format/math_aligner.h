#pragma once
#include <string>
#include <vector>
#include <algorithm>
#include "core/unicode_width.h"

namespace latex_fmt {

    inline bool isRuleCmdLine(const std::string& s) {
        return s == "\\hline"
            || s == "\\toprule"
            || s == "\\midrule"
            || s == "\\bottomrule"
            || s.rfind("\\hline[", 0) == 0
            || s.rfind("\\cmidrule", 0) == 0;
    }

    struct AlignCell {
        std::string content;
        int width;
    };

    struct AlignRow {
        std::vector<AlignCell> cells;
        std::string ending;
    };

    class MathAligner {
    public:
        static std::string trim(const std::string& str) {
            size_t first = str.find_first_not_of(" \t\n");
            if (first == std::string::npos) return "";
            size_t last = str.find_last_not_of(" \t\n");
            return str.substr(first, (last - first + 1));
        }

        static std::string align(const std::vector<std::string>& lines) {
            std::vector<AlignRow> rows;
            size_t max_cols = 0;

            for (const auto& line : lines) {
                AlignRow row;
                size_t pos = 0;
                std::string current_cell;

                while (pos < line.size()) {
                    if (line[pos] == '&') {
                        std::string trimmed = trim(current_cell);
                        row.cells.push_back({trimmed, display_width(trimmed)});
                        current_cell.clear();
                        pos++;
                    } else if (pos + 1 < line.size() && line[pos] == '\\' && line[pos+1] == '\\') {
                        std::string trimmed = trim(current_cell);
                        row.cells.push_back({trimmed, display_width(trimmed)});
                        current_cell.clear();
                        pos += 2;
                        while (pos < line.size() && line[pos] == ' ') pos++;
                        std::string ending = " \\\\";
                        if (pos < line.size() && line[pos] == '[') {
                            ending += '[';
                            pos++;
                            while (pos < line.size() && line[pos] != ']') {
                                ending += line[pos];
                                pos++;
                            }
                            if (pos < line.size()) {
                                ending += ']';
                                pos++;
                            }
                        }
                        row.ending = ending;
                        break;
                    } else {
                        current_cell += line[pos];
                        pos++;
                    }
                }

                if (!current_cell.empty() || row.ending.empty()) {
                    std::string trimmed = trim(current_cell);
                    row.cells.push_back({trimmed, display_width(trimmed)});
                }

                max_cols = std::max(max_cols, row.cells.size());
                rows.push_back(std::move(row));
            }

            std::vector<int> col_widths(max_cols, 0);
            for (const auto& row : rows) {
                if (row.ending.empty() && row.cells.size() == 1 &&
                    !row.cells[0].content.empty() && row.cells[0].content[0] == '%') {
                    continue;
                }
                if (row.cells.size() == 1 && isRuleCmdLine(row.cells[0].content)) {
                    continue;
                }
                for (size_t i = 0; i < row.cells.size(); ++i) {
                    col_widths[i] = std::max(col_widths[i], row.cells[i].width);
                }
            }

            std::string result;
            for (size_t r = 0; r < rows.size(); ++r) {
                const auto& row = rows[r];
                if (row.cells.size() == 1 && isRuleCmdLine(row.cells[0].content)) {
                    result += row.cells[0].content + row.ending;
                    if (r < rows.size() - 1) result += "\n";
                    continue;
                }
                for (size_t i = 0; i < row.cells.size(); ++i) {
                    result += row.cells[i].content;
                    if (i < row.cells.size() - 1) {
                        int padding = col_widths[i] - row.cells[i].width;
                        result += std::string(padding, ' ');
                        result += " & ";
                    }
                }
                result += row.ending;
                if (r < rows.size() - 1) result += "\n";
            }

            return result;
        }
    };

} // namespace latex_fmt
