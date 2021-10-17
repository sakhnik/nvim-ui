#pragma once

#include "Utils.hpp"
#include <gtk/gtk.h>

class GPointer
{
public:
    void Update(bool is_busy, GtkWidget *);

private:
    PtrT<GdkCursor> _active_cursor = NullPtr<GdkCursor>([](auto *c) { g_object_unref(c); });
    PtrT<GdkCursor> _busy_cursor = NullPtr<GdkCursor>([](auto *c) { g_object_unref(c); });
};
