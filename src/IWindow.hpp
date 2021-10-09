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

        bool ToBeRedrawn() const { return _redraw; }
        void MarkToRedraw(bool redraw = true) { _redraw = redraw; }

    private:
        bool _redraw = true;
    };

    virtual ITexture::PtrT CreateTexture(int width, std::string_view text, const HlAttr &, const HlAttr &def_attr) = 0;

    virtual void Present() = 0;
    virtual void SessionEnd() = 0;
};
