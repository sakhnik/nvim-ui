#pragma once

#include "IWindow.hpp"
#include "Utils.hpp"
#include "Session.hpp"
#include "GGrid.hpp"
#include <string>
#include <memory>
#include <gtk/gtk.h>


class GWindow
    : public IWindow
{
public:
    GWindow(GtkApplication *, Session::PtrT &session);
    ~GWindow();

    ITexture::PtrT CreateTexture(int width, std::string_view text, const HlAttr &, const HlAttr &def_attr) override;
    void Present() override;
    void DrawCursor(cairo_t *, int row, int col, unsigned fg, std::string_view mode);
    void SessionEnd() override;

    void SetError(const char *error) override;

private:
    GtkApplication *_app;
    Session::PtrT &_session;

    PtrT<GtkBuilder> _builder = NullPtr<GtkBuilder>([](auto *b) { g_object_unref(b); });
    GtkWidget *_window;
    GtkWidget *_scroll;
    std::unique_ptr<GGrid> _grid;

    int _last_rows = 0, _last_cols = 0;

    void _SetupWindow();
    void _SetupWindowSignals();
    void _SetupGrid();
    void _SetupStatusLabel();

    void _CheckSizeAsync();
    void _CheckSize();

    gboolean _OnKeyPressed(guint keyval, guint keycode, GdkModifierType state);
    gboolean _OnKeyReleased(guint keyval, guint keycode, GdkModifierType state);
    // Mark the Alt was pressed, show the menubar when the alt is released without any other key pressed.
    bool _alt_pending = false;

    void _Present();
    void _SessionEnd();
};
