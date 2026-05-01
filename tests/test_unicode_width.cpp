#include <catch2/catch_test_macros.hpp>
#include "core/unicode_width.h"

TEST_CASE("Unicode: decode_utf8", "[unicode]") {
    using namespace latex_fmt;

    SECTION("ASCII character") {
        size_t pos = 0;
        uint32_t cp = decode_utf8("A", pos);
        REQUIRE(cp == 0x41);
        REQUIRE(pos == 1);
    }

    SECTION("2-byte sequence (Latin extended)") {
        // U+00C1 = Á = 0xC3 0x81
        size_t pos = 0;
        uint32_t cp = decode_utf8("\xC3\x81", pos);
        REQUIRE(cp == 0x00C1);
        REQUIRE(pos == 2);
    }

    SECTION("3-byte sequence (CJK)") {
        // U+4E2D = 中 = 0xE4 0xB8 0xAD
        size_t pos = 0;
        uint32_t cp = decode_utf8("\xE4\xB8\xAD", pos);
        REQUIRE(cp == 0x4E2D);
        REQUIRE(pos == 3);
    }

    SECTION("4-byte sequence (emoji)") {
        // U+1F600 = 😀 = 0xF0 0x9F 0x98 0x80
        size_t pos = 0;
        uint32_t cp = decode_utf8("\xF0\x9F\x98\x80", pos);
        REQUIRE(cp == 0x1F600);
        REQUIRE(pos == 4);
    }

    SECTION("Empty string") {
        size_t pos = 0;
        uint32_t cp = decode_utf8("", pos);
        REQUIRE(cp == 0);
    }

    SECTION("Past end") {
        size_t pos = 5;
        uint32_t cp = decode_utf8("hello", pos);
        REQUIRE(cp == 0);
    }

    SECTION("Truncated 2-byte") {
        size_t pos = 0;
        uint32_t cp = decode_utf8("\xC3", pos); // missing continuation
        REQUIRE(cp == '?');
    }

    SECTION("Truncated 3-byte") {
        size_t pos = 0;
        uint32_t cp = decode_utf8("\xE4\xB8", pos); // missing continuation
        REQUIRE(cp == '?');
    }

    SECTION("Invalid continuation byte") {
        size_t pos = 0;
        uint32_t cp = decode_utf8("\xE4\x00\xAD", pos); // invalid continuation
        REQUIRE(cp == '?');
    }

    SECTION("Invalid leading byte") {
        size_t pos = 0;
        uint32_t cp = decode_utf8("\xFE", pos);
        REQUIRE(cp == '?');
    }

    SECTION("Sequential decoding") {
        std::string_view sv = "ABC";
        size_t pos = 0;
        REQUIRE(decode_utf8(sv, pos) == 'A');
        REQUIRE(pos == 1);
        REQUIRE(decode_utf8(sv, pos) == 'B');
        REQUIRE(pos == 2);
        REQUIRE(decode_utf8(sv, pos) == 'C');
        REQUIRE(pos == 3);
    }
}

TEST_CASE("Unicode: char_width", "[unicode]") {
    using namespace latex_fmt;

    SECTION("ASCII printable") {
        REQUIRE(char_width('A') == 1);
        REQUIRE(char_width('z') == 1);
        REQUIRE(char_width('0') == 1);
        REQUIRE(char_width(' ') == 1);
    }

    SECTION("CJK Unified Ideographs U+4E00..U+9FFF") {
        REQUIRE(char_width(0x4E2D) == 2); // 中
        REQUIRE(char_width(0x6587) == 2); // 文
        REQUIRE(char_width(0x4E00) == 2); // 一 (CJK)
    }

    SECTION("CJK Extension A U+3400..U+4DBF") {
        REQUIRE(char_width(0x3447) == 2);
        REQUIRE(char_width(0x4DB5) == 2);
    }

    SECTION("CJK Compatibility Ideographs U+F900..U+FAFF") {
        REQUIRE(char_width(0xF900) == 2);
        REQUIRE(char_width(0xFA00) == 2);
    }

    SECTION("CJK Symbols and Punctuation U+3000..U+303F") {
        REQUIRE(char_width(0x3000) == 2); // 全角空格
        REQUIRE(char_width(0x3001) == 2); // 、
        REQUIRE(char_width(0x3002) == 2); // 。
    }

    SECTION("Fullwidth Forms U+FF00..U+FFEF") {
        REQUIRE(char_width(0xFF01) == 2); // ！
        REQUIRE(char_width(0xFF21) == 2); // Ａ
        REQUIRE(char_width(0xFF41) == 2); // ａ
    }

    SECTION("Hangul Syllables U+AC00..U+D7AF") {
        REQUIRE(char_width(0xAC00) == 2); // 가
        REQUIRE(char_width(0xD7AF) == 2);
    }

    SECTION("Combining marks U+0300..U+036F have width 0") {
        REQUIRE(char_width(0x0300) == 0); // combining grave
        REQUIRE(char_width(0x0301) == 0); // combining acute
        REQUIRE(char_width(0x036F) == 0); // last combining mark
    }

    SECTION("Non-CJK non-combinging characters default to 1") {
        REQUIRE(char_width(0x00E9) == 1); // é (Latin small letter e with acute)
        REQUIRE(char_width(0x0410) == 1); // А (Cyrillic)
        REQUIRE(char_width(0x03B1) == 1); // α (Greek small letter alpha)
    }
}

TEST_CASE("Unicode: display_width", "[unicode]") {
    using namespace latex_fmt;

    SECTION("Empty string") {
        REQUIRE(display_width("") == 0);
    }

    SECTION("Pure ASCII") {
        REQUIRE(display_width("hello") == 5);
        REQUIRE(display_width("123") == 3);
        REQUIRE(display_width("a b") == 3);
    }

    SECTION("Pure CJK") {
        REQUIRE(display_width("中文") == 4);     // 2 + 2
        REQUIRE(display_width("你好世界") == 8); // 4 * 2
    }

    SECTION("Mixed ASCII and CJK") {
        REQUIRE(display_width("hello中文") == 9);  // 5 + 4
        REQUIRE(display_width("A中B") == 4);       // 1 + 2 + 1
    }

    SECTION("Combining mark does not add width") {
        // 'e' (U+0065) + combining acute accent (U+0301)
        std::string combined = "e\xCC\x81";
        REQUIRE(display_width(combined) == 1);
    }

    SECTION("Multiple combining marks") {
        // 'a' + 2 combining marks
        std::string combined = "a\xCC\x81\xCC\x80";
        REQUIRE(display_width(combined) == 1);
    }

    SECTION("CJK punctuation (width 2)") {
        // U+3001 = 、, U+3002 = 。
        std::string_view sv = "\xE3\x80\x81\xE3\x80\x82"; // 、、
        REQUIRE(display_width(sv) == 4);
    }

    SECTION("Mixed with combining marks") {
        // "中e\xCC\x81文" = 中(2) + e(1) + combining(0) + 文(2) = 5
        std::string mixed = "\xE4\xB8\xAD" "e\xCC\x81" "\xE6\x96\x87";
        REQUIRE(display_width(mixed) == 5);
    }

    SECTION("Fullwidth Latin") {
        // U+FF21 = Ａ (fullwidth A)
        std::string_view fw = "\xEF\xBC\xA1";
        REQUIRE(display_width(fw) == 2);
    }

    SECTION("Complex mixed string") {
        // "Hi中文!" = 1+1+2+2+1 = 7
        REQUIRE(display_width("Hi中文!") == 7);
    }

    SECTION("Emoji are width 1 (not CJK)") {
        // U+1F600 = 😀 = 4 bytes, width 1 per default rule
        std::string_view emoji = "\xF0\x9F\x98\x80";
        REQUIRE(display_width(emoji) == 1);
    }

    SECTION("Emoji mixed with ASCII") {
        std::string mixed = std::string("hi") + "\xF0\x9F\x98\x80" + std::string("there");
        REQUIRE(display_width(mixed) == 8); // 2 + 1 + 5
    }

    SECTION("Zero-width joiner (ZWJ)") {
        // U+200D = ZWJ, width should be 1 (default, not combining mark)
        std::string_view zwj = "\xE2\x80\x8D";
        REQUIRE(display_width(zwj) == 1);
    }

    SECTION("Tab character") {
        REQUIRE(display_width("\t") == 1);
    }

    SECTION("Newline character") {
        REQUIRE(display_width("\n") == 1);
    }
}
