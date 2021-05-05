#pragma once

#include "HlAttr.hpp"
#include <utility>
#include <memory>
#include <string_view>

struct IWindow
{
    virtual ~IWindow() = default;

    struct ITexture
    {
        using PtrT = std::shared_ptr<ITexture>;
        virtual ~ITexture() {}
    };

    virtual ITexture::PtrT CreateTexture(int width, std::string_view text, const HlAttr &, const HlAttr &def_attr) = 0;

    virtual void Present() = 0;
    virtual void SetBusy(bool is_busy) = 0;
};
