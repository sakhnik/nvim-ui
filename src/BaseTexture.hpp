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

    // Increment/decrement the reference counter to keep the texture alive.
    // Note that we can only create and manipulate Gtk widgets from the Gtk thread.
    // So we'll keep track of the life time without actual butchery.
    void IncRef(bool inc)
    {
        _refcount += inc ? 1 : -1;
    }

    // Increment/decrement the reference counter for the widget visibility.
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
    int _refcount = 0;
    static const int VISIBLE_INCREMENT = 10000;
};
