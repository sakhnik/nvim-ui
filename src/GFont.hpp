#pragma once

#include <Gtk/Window.hpp>

#include <string>
#include <functional>

namespace Gtk = gir::Gtk;

class GFont
{
public:
    GFont(Gtk::Window parent);

    const std::string& GetFamily() const { return _family; }
    double GetSizePt() const { return _size_pt; }
    void SetSizePt(double size_pt);
    void SetGuiFont(const std::string &);

    using OnFontChanged = std::function<void()>;
    void Subscribe(OnFontChanged on_changed)
    {
        _on_changed = on_changed;
    }

private:
    Gtk::Window _parent;
    std::string _family = "Monospace";
    double _size_pt = 14;
    OnFontChanged _on_changed;
};
