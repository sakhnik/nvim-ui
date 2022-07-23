#include "config.hpp"
#include "GSettingsDlg.hpp"
#include "GFont.hpp"
#include "GConfig.hpp"

#include <Gtk/Builder.hpp>
#include <Gtk/Dialog.hpp>
#include <Gtk/Button.hpp>
#include <Gtk/Label.hpp>
#include <Gtk/ToggleButton.hpp>

#ifdef GIR_INLINE
#include "Gtk/Dialog.ipp"
#include "Gtk/Builder.ipp"
#include <Gtk/Button.ipp>
#include <Gtk/Label.ipp>
#include <Gtk/ToggleButton.ipp>
#endif //GIR_INLINE

#include <fmt/format.h>

namespace Gtk = gir::Gtk;

namespace {

void HandleFont(Gtk::Builder builder, Gtk::Dialog dlg, GFont &font)
{
    Gtk::Label current_font_lbl{builder.get_object("current_font_lbl").g_obj()};

    auto update_current_font = [&font, current_font_lbl]() {
        std::string markup = fmt::format("<span font_desc=\"{0} {1:.1f}\">{0} {1:.1f}</span>",
                font.GetFamily(), font.GetSizePt());
        current_font_lbl.set_markup(markup.c_str());
    };
    update_current_font();

    int font_sub_id = font.Subscribe([update_current_font]() {
        update_current_font();
    });

    Gtk::Button font_btn{builder.get_object("choose_font_btn").g_obj()};
    font_btn.on_clicked(font_btn, [&font, dlg](Gtk::Button) {
        font.SetGuiFont("*", dlg);
    });

    dlg.on_destroy(dlg, [font_sub_id, &font](Gtk::Dialog) {
        font.Unsubscribe(font_sub_id);
    });
}

void HandleSmoothScroll(Gtk::Builder builder)
{
    Gtk::ToggleButton btn{builder.get_object("smooth_scroll_btn").g_obj()};
    auto updateLabel = [btn](bool is_active) {
        btn.set_label(is_active ? _("Disable") : _("Enable"));
    };
    bool is_active = GConfig::GetSmoothScroll();
    updateLabel(is_active);
    btn.set_label(is_active ? _("Disable") : _("Enable"));
    btn.set_active(is_active);
    btn.on_toggled(btn, [updateLabel](Gtk::ToggleButton btn) {
        bool is_active = btn.get_active();
        GConfig::SetSmoothScroll(is_active);
        updateLabel(is_active);
    });
}

} //namespace;

GSettingsDlg::GSettingsDlg(gir::Gtk::Window window, GFont &font)
{
    auto builder = Gtk::Builder::new_from_resource("/org/sakhnik/nvim-ui/gtk/settings-dlg.ui");
    Gtk::Dialog dlg{builder.get_object("settings_dlg").g_obj()};

    HandleFont(builder, dlg, font);
    HandleSmoothScroll(builder);

    dlg.set_transient_for(window);
    dlg.on_destroy(dlg, [builder](Gtk::Dialog) {
        builder.unref();
    });
    dlg.show();
}
