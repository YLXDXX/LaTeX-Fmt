#pragma once
#include <algorithm>
#include <sstream>
#include <string>
#include <vector>

namespace latex_fmt::utils {

inline std::vector<std::string> split_lines(const std::string& s) {
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

inline std::vector<std::vector<int>> compute_lcs_table(
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

inline std::string generate_unified_diff(
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

} // namespace latex_fmt::utils
