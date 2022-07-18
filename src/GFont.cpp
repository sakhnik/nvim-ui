#include "GFont.hpp"
#include "Logger.hpp"

#include <Gtk/FontChooserDialog.hpp>
#include <Gtk/Dialog.hpp>
#include <Gtk/ResponseType.hpp>

#ifdef GIR_INLINE
#include <Gtk/FontChooserDialog.ipp>
#endif


GFont::GFont(Gtk::Window parent)
    : _parent{parent}
{
}

void GFont::SetGuiFont(const std::string &value)
{
    if (value.empty())
        return;
    if (value[0] == '*')
    {
        using FCD = Gtk::FontChooserDialog;
        FCD dlg = FCD::new_("Choose the font", _parent).g_obj();
        dlg.set_font(fmt::format("{} {}", _family, _size_pt).c_str());
        dlg.set_level(GTK_FONT_CHOOSER_LEVEL_FAMILY);
        dlg.set_filter_func([](const PangoFontFamily *family, const PangoFontFace *, gpointer) -> gboolean {
                return pango_font_family_is_monospace(const_cast<PangoFontFamily *>(family));
            },
            nullptr, nullptr);
        dlg.on_response(dlg, [this](FCD d, gint response) {
            d.close();
            if (Gtk::ResponseType::ok == response)
            {
                _family = pango_font_family_get_name(d.get_font_family());
                _size_pt = 1. * d.get_font_size() / PANGO_SCALE;
                Logger().info("Font: {}:{}", _family, _size_pt);
                if (_on_changed)
                    _on_changed();
            }
            d.destroy();
        });
        dlg.show();
        return;
    }
    auto idx = value.find_first_of(":,");
    _family = value.substr(0, idx);
    Logger().info("Set guifont {}", _family);
    if (_on_changed)
        _on_changed();
}
