#pragma once
#include <cstddef>

namespace latex_fmt {

    struct SourceRange {
        size_t begin_offset = 0;
        size_t end_offset   = 0;
    };

} // namespace latex_fmt
