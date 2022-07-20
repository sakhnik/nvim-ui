#include "config.hpp"
#include "GConfig.hpp"

#include <Gio/SettingsSchemaSource.hpp>

#include <stdexcept>

#ifdef GIR_INLINE
#include <Gio/Settings.ipp>
#include <Gio/SettingsSchemaSource.ipp>
#endif

namespace Gio = gir::Gio;

GConfig::_SettingsT GConfig::_settings{nullptr};

void GConfig::Init(const std::string &settings_dir)
{
    auto schema_source = gir::MakeOwned(
            Gio::SettingsSchemaSource::new_from_directory(settings_dir.c_str(),
                Gio::SettingsSchemaSource::get_default(), false));
    auto schema = schema_source.lookup("com.sakhnik.nvim-ui", false);
    if (!schema.g_obj())
        throw std::runtime_error(_("Cannot get GSettings schema"));
    _settings = gir::MakeOwned(Gio::Settings::new_full(schema, nullptr, nullptr));
}
