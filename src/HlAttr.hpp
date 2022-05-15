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
        F_UNDERUNDERLINE = 1 << 4,
        F_UNDERCURL = 1 << 5,
        F_STRIKETHROUGH = 1 << 6,

        F_SPECIAL_COLORED = F_UNDERLINE | F_UNDERUNDERLINE | F_UNDERCURL,
    };
    std::optional<uint32_t> fg, bg;
    unsigned flags = 0;
    std::optional<uint32_t> special{};
};
