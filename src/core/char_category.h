#pragma once
#include <cstdint>
#include <string_view>
#include <cctype>
#include "unicode_width.h"

namespace latex_fmt {

enum class CharCategory { None, ASCII, CJK, CJKPunct, Space, Other };

inline CharCategory classify_cp(uint32_t cp) {
    if (cp == 0) return CharCategory::None;
    if (cp == ' ' || cp == '\t') return CharCategory::Space;
    if (cp == '~') return CharCategory::Other;
    if (cp < 0x80) {
        if (std::isalnum(static_cast<int>(cp))) return CharCategory::ASCII;
        return CharCategory::Other;
    }
    if ((cp >= 0x4E00 && cp <= 0x9FFF) ||
        (cp >= 0x3400 && cp <= 0x4DBF) ||
        (cp >= 0xF900 && cp <= 0xFAFF) ||
        (cp >= 0xAC00 && cp <= 0xD7AF)) return CharCategory::CJK;
    if ((cp >= 0x3000 && cp <= 0x303F) ||
        (cp >= 0xFF00 && cp <= 0xFFEF)) return CharCategory::CJKPunct;
    return CharCategory::Other;
}

inline CharCategory classify_first_char(std::string_view s) {
    size_t pos = 0;
    uint32_t cp = decode_utf8(s, pos);
    return classify_cp(cp);
}

inline CharCategory classify_last_char(std::string_view s) {
    size_t pos = 0;
    uint32_t last_cp = 0;
    while (pos < s.size()) {
        size_t prev = pos;
        uint32_t cp = decode_utf8(s, pos);
        if (cp == 0) break;
        if (cp != ' ' && cp != '\t') last_cp = cp;
    }
    return classify_cp(last_cp);
}

inline bool is_cjk_category(CharCategory cat) {
    return cat == CharCategory::CJK || cat == CharCategory::CJKPunct;
}

} // namespace latex_fmt
