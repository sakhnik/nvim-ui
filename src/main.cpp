#include "GWindow.hpp"
#include "SessionSpawn.hpp"
#include "Logger.hpp"
#include "gir/Owned.hpp"
#include "Gtk/Application.hpp"
#include "Gtk/Window.hpp"
#include <spdlog/cfg/env.h>

#include <uv.h>

namespace Gtk = gir::Gtk;

namespace {

    Session::PtrT session;
    std::unique_ptr<GWindow> window;

} //namespace;

int _argc;
char **_argv;

int main(int argc, char* argv[])
{
    _argc = argc;
    _argv = argv;

    setlocale(LC_CTYPE, "");
    spdlog::cfg::load_env_levels();

    Logger().info("nvim-ui v{}", VERSION);
    try
    {
        auto app = gir::MakeOwned(Gtk::Application::new_("org.nvim-ui", G_APPLICATION_FLAGS_NONE));
        app.set_resource_base_path("/org/nvim-ui");

        auto on_activate = [](Gtk::Application app) {
            std::string error;
            try
            {
                // Start the app by spawning a local nvim
                session.reset(new SessionSpawn(_argc, _argv));
            }
            catch (std::exception &ex)
            {
                error = ex.what();
            }
            window.reset(new GWindow{app, session});
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
            if (session)
            {
                // Resurrect the window if the session is still active
                window.reset(new GWindow{app, session});
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
