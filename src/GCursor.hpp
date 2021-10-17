#pragma once

#include "Session.hpp"
#include <gtk/gtk.h>

class GGrid;

class GCursor
{
public:
    GCursor(GtkWidget *, GGrid *, Session::PtrT &);

    void Move();
    void Hide();

private:
    GtkWidget *_cursor;
    GGrid *_grid;
    Session::PtrT &_session;

    void _DrawCursor(GtkDrawingArea *, cairo_t *cr, int /*width*/, int /*height*/);
};
