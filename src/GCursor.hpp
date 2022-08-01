#pragma once

#include "Session.hpp"

#include <Gtk/DrawingArea.hpp>

namespace Gtk = gir::Gtk;
class GGrid;

class GCursor
{
public:
    GCursor(Gtk::DrawingArea cursor, GGrid *, Session::AtomicPtrT &);

    void Move();
    void Hide();
    void UpdateSize();

private:
    Gtk::DrawingArea _cursor;
    GGrid *_grid;
    Session::AtomicPtrT &_session;

    void _DrawCursor(GtkDrawingArea *, cairo_t *cr, int /*width*/, int /*height*/);
};
