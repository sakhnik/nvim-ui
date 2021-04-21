#pragma once

#include "HlAttr.hpp"
#include <utility>
#include <memory>
#include <string_view>

struct IWindow
{
    virtual ~IWindow() = default;
    virtual void Init() = 0;
    virtual void Deinit() = 0;

    using RowsColsT = std::pair<int, int>;
    virtual RowsColsT GetRowsCols() const = 0;

    virtual void Clear(unsigned bg) = 0;

    struct ITexture
    {
        using PtrT = std::unique_ptr<ITexture>;
        virtual ~ITexture() {}
    };

    virtual void CopyTexture(int row, int col, ITexture *) = 0;
    virtual ITexture::PtrT CreateTexture(int width, std::string_view text, const HlAttr &, const HlAttr &def_attr) = 0;

    virtual void Present() = 0;
    virtual void DrawCursor(int row, int col, unsigned fg, std::string_view mode) = 0;
    virtual void SetBusy(bool is_busy) = 0;
};
