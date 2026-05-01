#pragma once

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
};

} // namespace latex_fmt
