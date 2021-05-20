#include "Window.hpp"
#include "Logger.hpp"
#include <spdlog/cfg/env.h>

#include <uv.h>

int main(int /*argc*/, char* /*argv*/[])
{
    setlocale(LC_CTYPE, "");
    spdlog::cfg::load_env_levels();

    Logger().info("nvim-ui v{}", VERSION);
    try
    {
        gtk_init();

        Window window;

        while (true /*session.IsRunning()*/)
        {
            g_main_context_iteration(g_main_context_default(), true);
        }
    }
    catch (std::exception& e)
    {
        Logger().error("Exception: {}", e.what());
    }

    return 0;
}
