#pragma once

#include "IWindow.hpp"
#include "Utils.hpp"
#include "Painter.hpp"
#include <mutex>
#include <gtk/gtk.h>

class Renderer;
class Input;

class Window
    : public IWindow
{
public:
    Window(Renderer *, Input *);
    ~Window();

    ITexture::PtrT CreateTexture(int width, std::string_view text, const HlAttr &, const HlAttr &def_attr) override;
    void Present() override;
    void DrawCursor(cairo_t *, int row, int col, unsigned fg, std::string_view mode);
    void SetBusy(bool is_busy) override;

private:
    Renderer *_renderer;
    Input * _input;
    GtkWidget *_window;
    GtkWidget *_grid;
    std::unique_ptr<Painter> _painter;

    static void _OnDraw(GtkDrawingArea *, cairo_t *cr, int width, int height, gpointer data);
    void _OnDraw2(cairo_t *cr, int width, int height);
    static void _OnResize(GtkDrawingArea *, int width, int height, gpointer data);
    void _OnResize2(int width, int height);
    static gboolean _OnKeyPressed(GtkEventControllerKey *,
                                  guint                  keyval,
                                  guint                  keycode,
                                  GdkModifierType        state,
                                  gpointer               data);
    gboolean _OnKeyPressed2(guint keyval, guint keycode, GdkModifierType state);

};
