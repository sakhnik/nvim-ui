#pragma once

#include <memory>

class BaseTexture
{
public:
    using PtrT = std::shared_ptr<BaseTexture>;
    virtual ~BaseTexture() {}

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
    int _refcount = 0;
    static const int VISIBLE_INCREMENT = 10000;
};
