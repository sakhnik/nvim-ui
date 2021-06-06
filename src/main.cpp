#include "Window.hpp"
#include "Session.hpp"
#include "Logger.hpp"
#include <spdlog/cfg/env.h>

#include <uv.h>

namespace {
    Session::PtrT session;
    std::unique_ptr<Window> window;
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
        GtkApplication *app = gtk_application_new("org.nvim-ui", G_APPLICATION_FLAGS_NONE);
        g_application_set_resource_base_path(G_APPLICATION(app), "/org/nvim-ui");

        using OnActivateT = void (*)(GtkApplication *);
        OnActivateT on_activate = [](auto *app) {
            std::string error;
            try
            {
                session.reset(new Session(_argc, _argv));
            }
            catch (std::exception &ex)
            {
                error = ex.what();
            }
            window.reset(new Window{app, session});
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

        g_application_run(G_APPLICATION(app), 0, nullptr);
    }
    catch (std::exception& e)
    {
        Logger().error("Exception: {}", e.what());
    }

    return 0;
}
