#include <catch2/catch_test_macros.hpp>
#include <string>
#include "md/md_converter.h"

static std::string convert(const std::string& input) {
    return latex_fmt::MdConverter().convert(input);
}

TEST_CASE("Markdown: headings", "[md]") {
    SECTION("H1 heading") {
        std::string result = convert("# Hello\n");
        REQUIRE(result.find("\\section{Hello}") != std::string::npos);
    }

    SECTION("H2 heading") {
        std::string result = convert("## Section\n");
        REQUIRE(result.find("\\subsection{Section}") != std::string::npos);
    }

    SECTION("H3 heading") {
        std::string result = convert("### Subsection\n");
        REQUIRE(result.find("\\subsubsection{Subsection}") != std::string::npos);
    }

    SECTION("H4 heading") {
        std::string result = convert("#### Paragraph\n");
        REQUIRE(result.find("\\paragraph{Paragraph}") != std::string::npos);
    }

    SECTION("H5 heading") {
        std::string result = convert("##### SubParagraph\n");
        REQUIRE(result.find("\\subparagraph{SubParagraph}") != std::string::npos);
    }

    SECTION("no heading for 6 #") {
        std::string result = convert("###### too many\n");
        REQUIRE(result.find("\\section") == std::string::npos);
        REQUIRE(result.find("\\subsection") == std::string::npos);
    }

    SECTION("no heading when # not followed by space") {
        std::string result = convert("#tag\n");
        REQUIRE(result.find("\\section") == std::string::npos);
    }
}

TEST_CASE("Markdown: emphasis", "[md]") {
    SECTION("bold with **") {
        std::string result = convert("this is **bold** text\n");
        REQUIRE(result.find("\\textbf{bold}") != std::string::npos);
    }

    SECTION("italic with *") {
        std::string result = convert("this is *italic* text\n");
        REQUIRE(result.find("\\textit{italic}") != std::string::npos);
    }

    SECTION("bold+italic with ***") {
        std::string result = convert("this is ***bold italic*** text\n");
        REQUIRE(result.find("\\textbf{\\textit{bold italic}}") != std::string::npos);
    }

    SECTION("bold with __") {
        std::string result = convert("this is __bold__ text\n");
        REQUIRE(result.find("\\textbf{bold}") != std::string::npos);
    }

    SECTION("italic with _") {
        std::string result = convert("this is _italic_ text\n");
        REQUIRE(result.find("\\textit{italic}") != std::string::npos);
    }
}

TEST_CASE("Markdown: code spans", "[md]") {
    SECTION("inline code") {
        std::string result = convert("use `printf()` to print\n");
        REQUIRE(result.find("\\texttt{printf()}") != std::string::npos);
    }

    SECTION("fenced code block") {
        std::string result = convert("```c\nint x = 1;\n```\n");
        REQUIRE(result.find("\\begin{verbatim}") != std::string::npos);
        REQUIRE(result.find("int x = 1;") != std::string::npos);
        REQUIRE(result.find("\\end{verbatim}") != std::string::npos);
    }

    SECTION("fenced code block with no language tag") {
        std::string result = convert("```\nhello\nworld\n```\n");
        REQUIRE(result.find("\\begin{verbatim}") != std::string::npos);
        REQUIRE(result.find("hello") != std::string::npos);
        REQUIRE(result.find("world") != std::string::npos);
        REQUIRE(result.find("\\end{verbatim}") != std::string::npos);
    }
}

TEST_CASE("Markdown: links", "[md]") {
    SECTION("simple link") {
        std::string result = convert("visit [GitHub](https://github.com)\n");
        REQUIRE(result.find("\\href{https://github.com}{GitHub}") != std::string::npos);
    }

    SECTION("link with formatting inside text") {
        std::string result = convert("see [**bold link**](url)\n");
        REQUIRE(result.find("\\href{url}{\\textbf{bold link}}") != std::string::npos);
    }

    SECTION("text with brackets but not a link") {
        std::string result = convert("array [0] is first\n");
        REQUIRE(result.find("\\href") == std::string::npos);
        bool has_bracket = (result.find("[0]") != std::string::npos
                         || result.find("\\[0\\]") != std::string::npos);
        REQUIRE(has_bracket);
    }
}

TEST_CASE("Markdown: lists", "[md]") {
    SECTION("unordered list with -") {
        std::string result = convert("- one\n- two\n- three\n");
        REQUIRE(result.find("\\begin{itemize}") != std::string::npos);
        REQUIRE(result.find("\\item one") != std::string::npos);
        REQUIRE(result.find("\\item two") != std::string::npos);
        REQUIRE(result.find("\\item three") != std::string::npos);
        REQUIRE(result.find("\\end{itemize}") != std::string::npos);
    }

    SECTION("unordered list with *") {
        std::string result = convert("* apple\n* banana\n");
        REQUIRE(result.find("\\begin{itemize}") != std::string::npos);
        REQUIRE(result.find("\\item apple") != std::string::npos);
        REQUIRE(result.find("\\item banana") != std::string::npos);
        REQUIRE(result.find("\\end{itemize}") != std::string::npos);
    }

    SECTION("ordered list") {
        std::string result = convert("1. first\n2. second\n3. third\n");
        REQUIRE(result.find("\\begin{enumerate}") != std::string::npos);
        REQUIRE(result.find("\\item first") != std::string::npos);
        REQUIRE(result.find("\\item second") != std::string::npos);
        REQUIRE(result.find("\\item third") != std::string::npos);
        REQUIRE(result.find("\\end{enumerate}") != std::string::npos);
    }

    SECTION("list items with inline formatting") {
        std::string result = convert("- **bold** item\n- *italic* item\n");
        REQUIRE(result.find("\\textbf{bold}") != std::string::npos);
        REQUIRE(result.find("\\textit{italic}") != std::string::npos);
    }

    SECTION("list separated by blank line starts new list") {
        std::string result = convert("- a\n\n- b\n");
        REQUIRE(result.find("\\begin{itemize}") != std::string::npos);
        REQUIRE(result.find("\\end{itemize}") != std::string::npos);
    }
}

TEST_CASE("Markdown: blockquotes", "[md]") {
    SECTION("single line blockquote") {
        std::string result = convert("> quoted text\n");
        REQUIRE(result.find("\\begin{quote}") != std::string::npos);
        REQUIRE(result.find("quoted text") != std::string::npos);
        REQUIRE(result.find("\\end{quote}") != std::string::npos);
    }

    SECTION("multi-line blockquote") {
        std::string result = convert("> line one\n> line two\n");
        REQUIRE(result.find("\\begin{quote}") != std::string::npos);
        REQUIRE(result.find("line one") != std::string::npos);
        REQUIRE(result.find("line two") != std::string::npos);
        REQUIRE(result.find("\\end{quote}") != std::string::npos);
    }

    SECTION("blockquote with inline formatting") {
        std::string result = convert("> **important** note\n");
        REQUIRE(result.find("\\begin{quote}") != std::string::npos);
        REQUIRE(result.find("\\textbf{important}") != std::string::npos);
        REQUIRE(result.find("\\end{quote}") != std::string::npos);
    }
}

TEST_CASE("Markdown: escaped characters", "[md]") {
    SECTION("escaped asterisk") {
        std::string result = convert("literal \\* asterisk\n");
        REQUIRE(result.find("\\textit") == std::string::npos);
        REQUIRE(result.find("asterisk") != std::string::npos);
    }

    SECTION("escaped underscore") {
        std::string result = convert("a \\_ b\n");
        REQUIRE(result.find("\\_") != std::string::npos);
        REQUIRE(result.find("\\textit") == std::string::npos);
    }

    SECTION("escaped backtick") {
        std::string result = convert("not \\` code\n");
        REQUIRE(result.find("\\texttt") == std::string::npos);
    }
}

TEST_CASE("Markdown: LaTeX special char escaping", "[md]") {
    SECTION("ampersand is escaped") {
        std::string result = convert("Tom & Jerry\n");
        REQUIRE(result.find("\\&") != std::string::npos);
    }

    SECTION("dollar sign is escaped") {
        std::string result = convert("costs $10\n");
        REQUIRE(result.find("\\$") != std::string::npos);
    }

    SECTION("percent is escaped") {
        std::string result = convert("50% off\n");
        REQUIRE(result.find("\\%") != std::string::npos);
    }

    SECTION("hash is escaped") {
        std::string result = convert("C# language\n");
        REQUIRE(result.find("\\#") != std::string::npos);
    }

    SECTION("existing LaTeX commands are preserved") {
        std::string result = convert("use \\textbf{bold} or \\emph{emphasis}\n");
        REQUIRE(result.find("\\textbf{bold}") != std::string::npos);
        REQUIRE(result.find("\\emph{emphasis}") != std::string::npos);
    }
}

TEST_CASE("Markdown: combined features", "[md]") {
    SECTION("heading with inline formatting") {
        std::string result = convert("# **Bold** Title\n");
        REQUIRE(result.find("\\section{\\textbf{Bold} Title}") != std::string::npos);
    }

    SECTION("paragraph with mixed formatting") {
        std::string result = convert("This **bold** and *italic* text.\n");
        REQUIRE(result.find("\\textbf{bold}") != std::string::npos);
        REQUIRE(result.find("\\textit{italic}") != std::string::npos);
    }

    SECTION("multiple paragraphs") {
        std::string result = convert("First paragraph.\n\nSecond paragraph.\n");
        REQUIRE(result.find("First paragraph.") != std::string::npos);
        REQUIRE(result.find("Second paragraph.") != std::string::npos);
    }

    SECTION("empty input") {
        std::string result = convert("");
        REQUIRE(result.empty());
    }
}

TEST_CASE("Markdown: CLI --md flag", "[cli][md]") {
    SECTION("--md flag recognized") {
        std::string result = latex_fmt::MdConverter().convert("# Test\n");
        REQUIRE(result.find("\\section{Test}") != std::string::npos);
    }
}
