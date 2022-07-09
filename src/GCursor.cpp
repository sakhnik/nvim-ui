#include "GCursor.hpp"
#include "GGrid.hpp"

#ifdef GIR_INLINE
#include <Gtk/DrawingArea.ipp>
#include <Gtk/Fixed.ipp>
#endif


GCursor::GCursor(Gtk::DrawingArea cursor, GGrid *grid, Session::PtrT &session)
    : _cursor{cursor}
    , _grid{grid}
    , _session{session}
{
    //<property name="can-focus">False</property>

    auto drawCursor = [](GtkDrawingArea *da, cairo_t *cr, int width, int height, gpointer data) {
        reinterpret_cast<GCursor *>(data)->_DrawCursor(da, cr, width, height);
    };
    cursor.set_draw_func(drawCursor, this, nullptr);
}

void GCursor::_DrawCursor(GtkDrawingArea *, cairo_t *cr, int /*width*/, int /*height*/)
{
    if (!_session)
        return;

    auto renderer = _session->GetRenderer();
    auto lock = renderer->Lock();
    double cell_width = _grid->CalcX(1);
    double cell_height = _grid->CalcY(1);

    cairo_save(cr);
    unsigned fg = renderer->GetFg();
    cairo_set_source_rgba(cr,
        static_cast<double>(fg >> 16) / 255,
        static_cast<double>((fg >> 8) & 0xff) / 255,
        static_cast<double>(fg & 0xff) / 255,
        0.5);

    const auto &mode = renderer->GetMode();
    if (mode == "insert")
    {
        cairo_rectangle(cr, 0, 0, 0.2 * cell_width, cell_height);
    }
    else if (mode == "replace" || mode == "operator")
    {
        cairo_rectangle(cr, 0, 0.75 * cell_height, cell_width, 0.25 * cell_height);
    }
    else
    {
        cairo_rectangle(cr, 0, 0, cell_width, cell_height);
    }
    cairo_fill(cr);
    cairo_restore(cr);
}

void GCursor::Move()
{
    auto renderer = _session->GetRenderer();

    Hide();
    if (!renderer->IsBusy())
    {
        // Move the cursor
        _grid->GetFixed().put(_cursor,
                _grid->CalcX(renderer->GetCursorCol()),
                _grid->CalcY(renderer->GetCursorRow()));
    }
}

void GCursor::Hide()
{
    if (_cursor.get_parent())
    {
        _cursor.ref();
        _grid->GetFixed().remove(_cursor);
    }
}

void GCursor::UpdateSize()
{
    _cursor.set_content_width(std::round(_grid->CalcX(1)));
    _cursor.set_content_height(_grid->CalcY(1));
}
