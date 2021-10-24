#pragma once

#include <memory>
#include <vector>

class BaseTexture
{
public:
    using PtrT = std::shared_ptr<BaseTexture>;
    virtual ~BaseTexture() {}

    // Check whether this texture is to be redrawn with the given
    // redraw token. Remove the token if present.
    bool TakeRedrawToken(uint32_t token);

    // Mark this texture to be redrawn with a redraw token.
    void MarkToRedraw(uint32_t token) { _redraw_tokens.push_back(token); }

    void IncRef(bool inc)
    {
        _refcount += inc ? 1 : -1;
    }

    void SetVisible(bool visible)
    {
        _refcount += visible ? VISIBLE_INCREMENT : -VISIBLE_INCREMENT;
    }

    bool IsVisible() const
    {
        return _refcount >= VISIBLE_INCREMENT;
    }

    bool IsAlive() const
    {
        return _refcount > 0;
    }

private:
    std::vector<uint32_t> _redraw_tokens;
    bool _destroy = false;
    int _refcount = 0;
    static const int VISIBLE_INCREMENT = 0; //0x10000;
};
