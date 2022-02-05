#pragma once

#include "HlAttr.hpp"
#include "BaseTexture.hpp"
#include <string_view>

struct IWindow
{
    virtual ~IWindow() = default;

    virtual BaseTexture::PtrT CreateTexture(int width, std::string_view text, const HlAttr &, const HlAttr &def_attr) = 0;

    virtual void Present(uint32_t token) = 0;
    virtual void SessionEnd() = 0;
    virtual void SetError(const char *) = 0;
    virtual void SetGuiFont(const std::string &) = 0;
};
