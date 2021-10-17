#pragma once

#include "Session.hpp"
#include "IWindow.hpp"
#include "Utils.hpp"
#include "GCursor.hpp"
#include "GPointer.hpp"
#include <vector>
#include <functional>
#include <gtk/gtk.h>

struct IMenuBarToggler;

class GGrid
{
public:
    GGrid(GtkWidget *grid, Session::PtrT &session, IMenuBarToggler *);

    GtkStyleProvider* GetStyle() const
    {
        return GTK_STYLE_PROVIDER(_css_provider.get());
    }

    void UpdateStyle();
    void MeasureCell();
    void Present(int width, int height);
    void Clear();
    void CheckSize(int width, int height);

    struct Texture : IWindow::ITexture
    {
        // Non-owning
        GtkWidget *widget = nullptr;
    };

    double CalcX(int col) const
    {
        return 1.0 * col * _cell_width / PANGO_SCALE;
    }

    double CalcY(int row) const
    {
        return row * _cell_height;
    }

    GtkFixed* GetFixed() const { return GTK_FIXED(_grid); }

private:
    GtkWidget *_grid;
    Session::PtrT &_session;
    IMenuBarToggler *_menu_bar_toggler;
    PtrT<GtkCssProvider> _css_provider = NullPtr<GtkCssProvider>([](auto *p) { g_object_unref(p); });

    int _cell_width = 0;
    int _cell_height = 0;
    std::vector<IWindow::ITexture::PtrT> _textures;

    std::unique_ptr<GCursor> _cursor;
    GPointer _pointer;

    gboolean _OnKeyPressed(guint keyval, guint /*keycode*/, GdkModifierType state);
    gboolean _OnKeyReleased(guint keyval, guint /*keycode*/, GdkModifierType /*state*/);
    // Mark the Alt was pressed, show the menubar when the alt is released without any other key pressed.
    bool _alt_pending = false;

    int _last_rows = 0, _last_cols = 0;
    void _CheckSize(int width, int height);
};
