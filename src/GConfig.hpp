#pragma once

#include <gir/Owned.hpp>
#include <Gio/Settings.hpp>
#include <Gio/SettingsSchema.hpp>

class GConfig
{
public:
    static void Init(const std::string &settings_dir);

    static gir::Gio::Settings& GetSettings() { return _settings; }
    static gir::Gio::SettingsSchema& GetSettingsSchema() { return _settings_schema; }

    static constexpr const char *FONT_FAMILY_KEY = "font-family";
    static constexpr const char *FONT_SIZE_KEY = "font-size";
    static constexpr const char *SMOOTH_SCROLL_DELAY_KEY = "smooth-scroll-delay";

    static std::string GetFontFamily();
    static void SetFontFamily(const std::string &);
    static double GetFontSize();
    static void SetFontSize(double pt);

    // Milliseconds between scroll steps; if 0, no smooth scroll
    static int GetSmoothScrollDelay();
    static void SetSmoothScrollDelay(int);

private:
    using _SettingsSchemaT = gir::Owned<gir::Gio::SettingsSchema>;
    static _SettingsSchemaT _settings_schema;
    using _SettingsT = gir::Owned<gir::Gio::Settings>;
    static _SettingsT _settings;
};
