#pragma once

#include "Session.hpp"
#include "IWindow.hpp"
#include "Utils.hpp"
#include "GCursor.hpp"
#include "GPointer.hpp"

#include "Gtk/Fixed.hpp"
#include "Gtk/Label.hpp"

#include <vector>
#include <functional>

namespace Gtk = gir::Gtk;
struct IWindowHandler;

class GGrid
{
public:
    GGrid(Gtk::Fixed grid, Session::PtrT &session, IWindowHandler *);

    GtkStyleProvider* GetStyle() const
    {
        return GTK_STYLE_PROVIDER(_css_provider.get());
    }

    void UpdateStyle();
    void MeasureCell();
    void Present(int width, int height, uint32_t token);
    void Clear();
    void CheckSize(int width, int height);

    struct Texture : BaseTexture
    {
        // Non-owning
        Gtk::Label label;
    };

    double CalcX(int col) const
    {
        return 1.0 * col * _cell_width / PANGO_SCALE;
    }

    double CalcY(int row) const
    {
        return row * _cell_height;
    }

    Gtk::Fixed GetFixed() const { return _grid; }

private:
    Gtk::Fixed _grid;
    Session::PtrT &_session;
    IWindowHandler *_window_handler;
    PtrT<GtkCssProvider> _css_provider = NullPtr<GtkCssProvider>([](auto *p) { g_object_unref(p); });

    int _cell_width = 0;
    int _cell_height = 0;
    std::vector<BaseTexture::PtrT> _textures;

    std::unique_ptr<GCursor> _cursor;
    GPointer _pointer;

    gboolean _OnKeyPressed(guint keyval, guint /*keycode*/, GdkModifierType state);
    gboolean _OnKeyReleased(guint keyval, guint /*keycode*/, GdkModifierType /*state*/);
    // Mark the Alt was pressed, show the menubar when the alt is released without any other key pressed.
    bool _alt_pending = false;

    int _last_rows = 0, _last_cols = 0;
    void _CheckSize(int width, int height);

    double _font_size_pt = 14;
};
