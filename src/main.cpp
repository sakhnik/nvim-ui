#include "Window.hpp"
#include "Session.hpp"
#include "Logger.hpp"
#include <spdlog/cfg/env.h>

#include <uv.h>

namespace {
    Session::PtrT session;
    std::unique_ptr<Window> window;
} //namespace;

int main(int argc, char* argv[])
{
    setlocale(LC_CTYPE, "");
    spdlog::cfg::load_env_levels();

    Logger().info("nvim-ui v{}", VERSION);
    try
    {
        GtkApplication *app = gtk_application_new("org.nvim-ui", G_APPLICATION_FLAGS_NONE);

        using OnActivateT = void (*)(GtkApplication *);
        OnActivateT on_activate = [](auto *app) {
            window.reset(new Window{app, session});
        };
        g_signal_connect(app, "activate", G_CALLBACK(on_activate), nullptr);

        using OnWindowRemovedT = void (*)(GtkApplication *, GtkWindow *, gpointer);
        OnWindowRemovedT on_window_removed = [](auto *app, auto *, gpointer) {
            if (session)
            {
                // Resurrect the window if the session is still active
                window.reset(new Window{app, session});
                // TODO: give some hint to quit neovim properly
            }
        };
        g_signal_connect(app, "window-removed", G_CALLBACK(on_window_removed), nullptr);

        session.reset(new Session(argc, argv));
        session->RunAsync();

        g_application_run(G_APPLICATION(app), argc, argv);
    }
    catch (std::exception& e)
    {
        Logger().error("Exception: {}", e.what());
    }

    return 0;
}
