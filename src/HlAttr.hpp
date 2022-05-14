#pragma once

#include <optional>
#include <cstdint>

struct HlAttr
{
    enum Flags
    {
        F_BOLD = 1 << 0,
        F_REVERSE = 1 << 1,
        F_ITALIC = 1 << 2,
        F_UNDERLINE = 1 << 3,
        F_UNDERCURL = 1 << 4,
    };
    std::optional<uint32_t> fg, bg;
    unsigned flags = 0;
    std::optional<uint32_t> special{};
};
