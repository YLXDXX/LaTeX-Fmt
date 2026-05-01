#pragma once
#include <string_view>
#include <cstdint>

namespace latex_fmt {

    inline uint32_t decode_utf8(std::string_view str, size_t& pos) {
        if (pos >= str.size()) return 0;
        uint8_t c = static_cast<uint8_t>(str[pos++]);

        uint32_t cp = 0;
        int bytes = 0;

        if ((c & 0x80) == 0) { return c; }
        else if ((c & 0xE0) == 0xC0) { cp = c & 0x1F; bytes = 1; }
        else if ((c & 0xF0) == 0xE0) { cp = c & 0x0F; bytes = 2; }
        else if ((c & 0xF8) == 0xF0) { cp = c & 0x07; bytes = 3; }
        else { return '?'; } // Invalid UTF-8

        for (int i = 0; i < bytes; ++i) {
            if (pos >= str.size() || (static_cast<uint8_t>(str[pos]) & 0xC0) != 0x80) {
                return '?'; // Malformed
            }
            cp = (cp << 6) | (static_cast<uint8_t>(str[pos]) & 0x3F);
            pos++;
        }
        return cp;
    }

    inline int char_width(uint32_t cp) {
        // Combining marks (U+0300..U+036F)
        if (cp >= 0x0300 && cp <= 0x036F) return 0;

        // CJK Unified Ideographs (U+4E00..U+9FFF)
        // CJK Extension A (U+3400..U+4DBF)
        // CJK Compatibility Ideographs (U+F900..U+FAFF)
        // CJK Symbols and Punctuation (U+3000..U+303F)
        // Fullwidth Forms (U+FF00..U+FFEF)
        // Hangul Syllables (U+AC00..U+D7AF)
        if ((cp >= 0x4E00 && cp <= 0x9FFF) ||
            (cp >= 0x3400 && cp <= 0x4DBF) ||
            (cp >= 0xF900 && cp <= 0xFAFF) ||
            (cp >= 0x3000 && cp <= 0x303F) ||
            (cp >= 0xFF00 && cp <= 0xFFEF) ||
            (cp >= 0xAC00 && cp <= 0xD7AF)) {
            return 2;
            }

            // Default for ASCII and most other characters
            return 1;
    }

    inline int display_width(std::string_view str) {
        int width = 0;
        size_t pos = 0;
        while (pos < str.size()) {
            uint32_t cp = decode_utf8(str, pos);
            if (cp == 0) break;
            width += char_width(cp);
        }
        return width;
    }

} // namespace latex_fmt
