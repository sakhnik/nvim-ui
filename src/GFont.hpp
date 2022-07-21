#pragma once

#include <Gtk/Window.hpp>

#include <string>
#include <functional>
#include <unordered_map>

namespace Gtk = gir::Gtk;

class GFont
{
public:
    GFont(Gtk::Window parent);

    const std::string& GetFamily() const { return _family; }
    double GetSizePt() const { return _size_pt; }
    void SetSizePt(double size_pt);
    void SetGuiFont(const std::string &);
    void SetGuiFont(const std::string &, Gtk::Window parent);

    using OnFontChanged = std::function<void()>;

    int Subscribe(OnFontChanged on_changed)
    {
        while (_subs.contains(_sub_id))
            ++_sub_id;
        _subs[_sub_id] = on_changed;
        return _sub_id;
    }

    void Unsubscribe(int sub_id)
    {
        _subs.erase(sub_id);
    }

private:
    Gtk::Window _parent;
    std::string _family = "Monospace";
    double _size_pt = 14;
    std::unordered_map<int, OnFontChanged> _subs;
    int _sub_id{};

    void _OnChanged();
};
