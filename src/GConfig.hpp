#pragma once

#include <gir/Owned.hpp>
#include <Gio/Settings.hpp>

class GConfig
{
public:
    static void Init(const std::string &settings_dir);

    static gir::Gio::Settings& GetSettings() { return _settings; }

private:
    using _SettingsT = gir::Owned<gir::Gio::Settings>;
    static _SettingsT _settings;
};
