#include "config.hpp"
#include "GConfig.hpp"
#include "Utils.hpp"

#include <Gio/SettingsSchemaSource.hpp>

#include <stdexcept>

#ifdef GIR_INLINE
#include <Gio/Settings.ipp>
#include <Gio/SettingsSchema.ipp>
#include <Gio/SettingsSchemaSource.ipp>
#endif

namespace Gio = gir::Gio;

GConfig::_SettingsT GConfig::_settings{nullptr};
GConfig::_SettingsSchemaT GConfig::_settings_schema{nullptr};

void GConfig::Init(const std::string &settings_dir)
{
    auto schema_source = gir::MakeOwned(
            Gio::SettingsSchemaSource::new_from_directory(settings_dir.c_str(),
                Gio::SettingsSchemaSource::get_default(), false));
    auto schema = schema_source.lookup("org.sakhnik.nvim-ui", false);
    if (!schema.g_obj())
        throw std::runtime_error(_("Cannot get GSettings schema"));
    _settings_schema = MakeOwned(schema.ref());
    _settings = gir::MakeOwned(Gio::Settings::new_full(schema, nullptr, nullptr));
}

std::string GConfig::GetFontFamily()
{
    // TODO: Make the wrapper return an auto-managed gchar*
    auto font_family = mk_unique_ptr(GConfig::GetSettings().get_string(FONT_FAMILY_KEY), free);
    std::string ret = font_family.get();
    return ret;
}

void GConfig::SetFontFamily(const std::string &font_family)
{
    _settings.set_string(FONT_FAMILY_KEY, font_family.c_str());
}

double GConfig::GetFontSize()
{
    return _settings.get_double(FONT_SIZE_KEY);
}

void GConfig::SetFontSize(double pt)
{
    _settings.set_double(FONT_SIZE_KEY, pt);
}

int GConfig::GetSmoothScrollDelay()
{
    return _settings.get_int(SMOOTH_SCROLL_DELAY_KEY);
}

void GConfig::SetSmoothScrollDelay(int ms)
{
    _settings.set_int(SMOOTH_SCROLL_DELAY_KEY, ms);
}

double GConfig::GetCellHeightAdjustment()
{
    return _settings.get_double(CELL_HEIGHT_ADJUSTMENT_KEY);
}

void GConfig::SetCellHeightAdjustment(double v)
{
    _settings.set_double(CELL_HEIGHT_ADJUSTMENT_KEY, v);
}
