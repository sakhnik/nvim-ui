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

        // Check whether this texture is to be redrawn with the given
        // redraw token. Remove the token if present.
        bool TakeRedrawToken(uint32_t token)
        {
            auto it = std::remove(_redraw_tokens.begin(), _redraw_tokens.end(), token);
            if (it == _redraw_tokens.end())
                return false;
            _redraw_tokens.erase(it, _redraw_tokens.end());
            return true;
        }

        // Mark this texture to be redrawn with a redraw token.
        void MarkToRedraw(uint32_t token) { _redraw_tokens.push_back(token); }

        bool ToBeDestroyed() const { return _destroy; }
        void MarkToDestroy(bool destroy = true) { _destroy = destroy; }

    private:
        std::vector<uint32_t> _redraw_tokens;
        bool _destroy = false;
    };

    virtual ITexture::PtrT CreateTexture(int width, std::string_view text, const HlAttr &, const HlAttr &def_attr) = 0;

    virtual void Present(uint32_t token) = 0;
    virtual void SessionEnd() = 0;
    virtual void SetError(const char *) = 0;
};
