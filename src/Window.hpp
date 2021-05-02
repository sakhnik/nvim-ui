#pragma once

#include "IWindow.hpp"
#include "Utils.hpp"
#include "Painter.hpp"
#include <mutex>
#include <gtk/gtk.h>


class Window
    : public IWindow
{
public:
    Window();
    ~Window();

    RowsColsT GetRowsCols() const override;

    void Clear(unsigned bg) override;
    void CopyTexture(int row, int col, ITexture *) override;
    ITexture::PtrT CreateTexture(int width, std::string_view text, const HlAttr &, const HlAttr &def_attr) override;
    void Present() override;
    void DrawCursor(int row, int col, unsigned fg, std::string_view mode) override;
    void SetBusy(bool is_busy) override;

private:
    GtkWidget *_window;
    GtkWidget *_grid;
    std::unique_ptr<Painter> _painter;
    PtrT<cairo_surface_t> _surface = NullPtr(cairo_surface_destroy);
    PtrT<cairo_t> _cairo = NullPtr(cairo_destroy);
    std::mutex _mut;

    static void _OnDraw(GtkDrawingArea *, cairo_t *cr, int width, int height, gpointer data);
    void _OnDraw2(cairo_t *cr, int width, int height);
    static void _OnResize(GtkDrawingArea *, int width, int height, gpointer data);
    void _OnResize2(int width, int height);
};
