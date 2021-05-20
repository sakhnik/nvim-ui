#pragma once

#include "IWindow.hpp"
#include "Utils.hpp"
#include "Session.hpp"
#include <unordered_set>
#include <string>
#include <memory>
#include <gtk/gtk.h>


class Window
    : public IWindow
{
public:
    Window();
    ~Window();

    ITexture::PtrT CreateTexture(int width, std::string_view text, const HlAttr &, const HlAttr &def_attr) override;
    void Present() override;
    void DrawCursor(cairo_t *, int row, int col, unsigned fg, std::string_view mode);
    void SessionEnd() override;

private:
    std::unique_ptr<Session> _session;

    GtkBuilder *_builder;
    GtkWidget *_window;
    GtkWidget *_scroll;
    GtkWidget *_grid;
    GtkWidget *_cursor;

    int _cell_width = 0, _cell_height = 0;
    int _last_rows = 0, _last_cols = 0;
    std::unordered_set<GtkWidget *> _widgets;
    PtrT<GtkCssProvider> _css_provider = NullPtr<GtkCssProvider>([](auto *p) { g_object_unref(p); });
    std::string _style;

    PtrT<GdkCursor> _active_cursor = NullPtr<GdkCursor>([](auto *c) { g_object_unref(c); });
    PtrT<GdkCursor> _busy_cursor = NullPtr<GdkCursor>([](auto *c) { g_object_unref(c); });

    void _CheckSizeAsync();
    void _CheckSize();

    gboolean _OnKeyPressed(guint keyval, guint keycode, GdkModifierType state);

    void _Present();
    void _UpdateStyle();
    void _DrawCursor(GtkDrawingArea *, cairo_t *, int width, int height);
    void _SessionEnd();
};
