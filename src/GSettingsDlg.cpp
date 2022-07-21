#include "GSettingsDlg.hpp"
#include "GFont.hpp"

#include <Gtk/Builder.hpp>
#include <Gtk/Dialog.hpp>
#include <Gtk/Button.hpp>
#include <Gtk/Label.hpp>

#ifdef GIR_INLINE
#include "Gtk/Dialog.ipp"
#include "Gtk/Builder.ipp"
#include <Gtk/Button.ipp>
#include <Gtk/Label.hpp>
#endif //GIR_INLINE

#include <fmt/format.h>

namespace Gtk = gir::Gtk;

GSettingsDlg::GSettingsDlg(gir::Gtk::Window window, GFont &font)
{
    auto builder = Gtk::Builder::new_from_resource("/org/sakhnik/nvim-ui/gtk/settings-dlg.ui");
    Gtk::Dialog dlg{builder.get_object("settings_dlg").g_obj()};
    Gtk::Label current_font_lbl{builder.get_object("current_font_lbl").g_obj()};

    auto update_current_font = [&font, current_font_lbl]() {
        Gtk::Label l{current_font_lbl};
        std::string markup = fmt::format("<span font_desc=\"{0} {1:.1f}\">{0} {1:.1f}</span>",
                font.GetFamily(), font.GetSizePt());
        l.set_markup(markup.c_str());
    };
    update_current_font();

    int font_sub_id = font.Subscribe([update_current_font]() {
        update_current_font();
    });

    Gtk::Button font_btn{builder.get_object("choose_font").g_obj()};
    font_btn.on_clicked(font_btn, [&font, dlg](Gtk::Button) {
        Gtk::Dialog d{dlg};
        font.SetGuiFont("*", d);
    });

    dlg.set_transient_for(window);
    dlg.on_destroy(dlg, [builder, font_sub_id, &font](Gtk::Dialog) {
        font.Unsubscribe(font_sub_id);
        Gtk::Builder b{builder};
        b.unref();
    });
    dlg.show();
}
