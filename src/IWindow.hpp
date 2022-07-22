#pragma once

#include "HlAttr.hpp"
#include <string>

struct IWindow
{
    virtual ~IWindow() = default;

    virtual void Present() = 0;
    virtual void SessionEnd() = 0;
    virtual void SetError(const char *) = 0;
    virtual void SetGuiFont(const std::string &) = 0;
};
