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
    Window(GtkApplication *, Session::PtrT &session);
    ~Window();

    ITexture::PtrT CreateTexture(int width, std::string_view text, const HlAttr &, const HlAttr &def_attr) override;
    void Present() override;
    void DrawCursor(cairo_t *, int row, int col, unsigned fg, std::string_view mode);
    void SessionEnd() override;

    void SetError(const char *error);

private:
    GtkApplication *_app;
    Session::PtrT &_session;

    PtrT<GtkBuilder> _builder = NullPtr<GtkBuilder>([](auto *b) { g_object_unref(b); });
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

    void _SetupWindow();
    void _SetupWindowSignals();
    void _SetupGrid();
    void _SetupCursor();
    void _SetupStatusLabel();

    void _CheckSizeAsync();
    void _CheckSize();
    void _MeasureCell();

    gboolean _OnKeyPressed(guint keyval, guint keycode, GdkModifierType state);
    gboolean _OnKeyReleased(guint keyval, guint keycode, GdkModifierType state);
    // Mark the Alt was pressed, show the menubar when the alt is released without any other key pressed.
    bool _alt_pending = false;

    void _Present();
    void _UpdateStyle();
    void _DrawCursor(GtkDrawingArea *, cairo_t *, int width, int height);
    void _SessionEnd();
};
