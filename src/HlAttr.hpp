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
        F_STRIKETHROUGH = 1 << 3,
        F_UNDERLINE = 1 << 4,
        F_UNDERUNDERLINE = 1 << 5,
        F_UNDERCURL = 1 << 6,
        F_UNDERDASH = 1 << 7,
        F_UNDERDOT = 1 << 8,

        F_TEXT_DECORATION = F_UNDERLINE | F_UNDERUNDERLINE | F_UNDERCURL | F_UNDERDASH | F_UNDERDOT,
    };
    std::optional<uint32_t> fg, bg;
    unsigned flags = 0;
    std::optional<uint32_t> special{};
};
