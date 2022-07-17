#pragma once

#include "Session.hpp"
#include "IWindow.hpp"
#include "Utils.hpp"
#include "GCursor.hpp"

#include "gir/Owned.hpp"
#include "Gtk/CssProvider.hpp"
#include "Gtk/Fixed.hpp"
#include "Gtk/Label.hpp"

#include <vector>
#include <functional>

namespace Gtk = gir::Gtk;
struct IWindowHandler;

class GFont;

class GGrid
{
public:
    GGrid(Gtk::Fixed grid, GFont &font, Session::PtrT &session, IWindowHandler *);

    Gtk::StyleProvider& GetStyle()
    {
        return _css_provider.get();
    }

    void UpdateStyle();
    void MeasureCell();
    void Present(int width, int height);
    void Clear();
    void CheckSize(int width, int height);

    struct Texture : BaseTexture
    {
        // Non-owning
        Gtk::Label label;
    };

    double CalcX(int col) const
    {
        return col * _cell_width;
    }

    double CalcY(int row) const
    {
        return row * _cell_height;
    }

    Gtk::Fixed GetFixed() const { return _grid; }

private:
    Gtk::Fixed _grid;
    GFont &_font;
    Session::PtrT &_session;
    IWindowHandler *_window_handler;
    gir::Owned<Gtk::CssProvider> _css_provider;
    std::unordered_map<unsigned, std::string> _pango_styles;
    std::string _default_pango_style;

    double _cell_width{};
    int _cell_height{};
    std::vector<BaseTexture::PtrT> _textures;

    std::unique_ptr<GCursor> _cursor;

    gboolean _OnKeyPressed(guint keyval, guint /*keycode*/, GdkModifierType state);
    void _OnKeyReleased(guint keyval, guint /*keycode*/, GdkModifierType /*state*/);
    // Mark the Alt was pressed, show the menubar when the alt is released without any other key pressed.
    bool _alt_pending = false;

    int _last_rows = 0, _last_cols = 0;
    void _CheckSize(int width, int height);
    void _RemoveOutdated();
    void _CreateLabels();
    void _CheckConsistency();
    void _UpdatePangoStyles();
    std::string _MakePangoStyle(const HlAttr &, const HlAttr &def_attr);
};
