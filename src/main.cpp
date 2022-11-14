#include "config.hpp"
#include "GWindow.hpp"
#include "GConfig.hpp"
#include "SessionSpawn.hpp"
#include "Logger.hpp"
#include <gir/Owned.hpp>

#include <Gtk/Application.hpp>
#include <Gtk/Window.hpp>
#include <Gio/ApplicationFlags.hpp>

#include <filesystem>
#include <spdlog/cfg/env.h>
#include <uv.h>

#ifdef GIR_INLINE
#include <Gtk/Application.ipp>
#endif

namespace Gtk = gir::Gtk;
namespace Gio = gir::Gio;

namespace {

Session::AtomicPtrT global_session;
std::unique_ptr<GWindow> window;

std::string GetSettingsDir()
{
    return ResourceDir::Get("res", std::filesystem::path{DATA_DIR} / "glib-2.0" / "schemas");
}

std::string GetLocalePath()
{
    return ResourceDir::Get("po", std::filesystem::path{DATA_DIR} / "locale");
}

} //namespace;

int _argc;
char **_argv;

int main(int argc, char* argv[])
{
    _argc = argc;
    _argv = argv;

    setlocale(LC_CTYPE, "");
    setlocale(LC_MESSAGES, "");
    spdlog::cfg::load_env_levels();
    Logger().info("nvim-ui v{}", VERSION);

    ResourceDir::Initialize(argv[0]);

    auto locale_path = GetLocalePath();
    Logger().info("Using locale path {}", locale_path);
    bindtextdomain(GETTEXT_PACKAGE, locale_path.c_str());
    textdomain(GETTEXT_PACKAGE);
    bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");

    auto settings_dir = GetSettingsDir();
    Logger().info("Using settings directory {}", settings_dir);
    GConfig::Init(settings_dir);

    try
    {
        auto app = gir::MakeOwned(Gtk::Application::new_("org.sakhnik.nvim-ui", Gio::ApplicationFlags::default_flags));
        app.set_resource_base_path("/org/sakhnik/nvim-ui");

        auto on_activate = [](Gtk::Application app) {
            // Only activate once. If a subsequent launch happens, nothing to do in this process.
            auto session = global_session.load();
            if (session)
                return;

            std::string error;
            try
            {
                // Start the app by spawning a local nvim
                session.reset(new SessionSpawn(_argc, _argv));
                global_session.store(session);
            }
            catch (std::exception &ex)
            {
                error = ex.what();
            }
            window.reset(new GWindow{app, global_session});
            if (session)
            {
                window->SetError(nullptr);
                session->SetWindow(window.get());
                session->RunAsync();
            }
            else
            {
                window->SetError(error.data());
            }
        };
        app.on_activate(app.get(), on_activate);

        auto on_window_removed = [](Gtk::Application app, Gtk::Window) {
            auto session = global_session.load();
            if (session)
            {
                // Resurrect the window if the global_session is still active
                window.reset(new GWindow{app, global_session});
                session->SetWindow(window.get());
                // TODO: give some hint to quit neovim properly
            }
        };
        app.on_window_removed(app.get(), on_window_removed);

        app.run(0, nullptr);
    }
    catch (std::exception& e)
    {
        Logger().error("Exception: {}", e.what());
    }

    return 0;
}
