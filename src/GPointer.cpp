#include "GPointer.hpp"


void GPointer::Update(bool is_busy, GtkWidget *grid)
{
    if (is_busy)
    {
        if (!_busy_cursor)
            _busy_cursor.reset(gdk_cursor_new_from_name("progress", nullptr));
        gtk_widget_set_cursor(grid, _busy_cursor.get());
    }
    else
    {
        if (!_active_cursor)
            _active_cursor.reset(gdk_cursor_new_from_name("default", nullptr));
        gtk_widget_set_cursor(grid, _active_cursor.get());
    }
}
