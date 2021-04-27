#include "MsgPackRpc.hpp"
#include "Renderer.hpp"
#include "RedrawHandler.hpp"
#include "Window.hpp"
#include "SdlLoop.hpp"
#include "Logger.hpp"
#include <spdlog/cfg/env.h>
#include <iostream>

#include <uv.h>

class App
{
public:
    App(uv_loop_t *loop, MsgPackRpc &rpc)
        : _rpc{rpc}
        , _timer{loop}
        , _renderer(&_rpc, &_window, &_timer)
        , _redraw_handler(&_rpc, &_renderer)
        , _sdl_loop{loop, &_renderer}
    {
        if (!_rpc.Activate())
            return;
        _redraw_handler.AttachUI();
        _sdl_loop.Start();
    }

private:
    MsgPackRpc &_rpc;
    Window _window;
    Timer _timer;
    Renderer _renderer;
    RedrawHandler _redraw_handler;

    SdlLoop _sdl_loop;
};

int main(int argc, char* argv[])
{
    setlocale(LC_CTYPE, "");
    spdlog::cfg::load_env_levels();

    Logger().info("nvim-ui v{}", VERSION);
    try
    {
        auto loop = uv_default_loop();

        //auto signal = loop->resource<uvw::SignalHandle>();
        //signal->once<uvw::SignalEvent>([&loop](const uvw::SignalEvent &ev, const auto &handle) {
        //    if (ev.signum == SIGINT || ev.signum == SIGTERM)
        //    {
        //        loop->close();
        //    }
        //});

        std::string nvim("nvim");
        std::string embed("--embed");

        std::vector<char *> args;
        args.push_back(nvim.data());
        args.push_back(embed.data());
        for (int i = 1; i < argc; ++i)
            args.push_back(argv[i]);
        args.push_back(nullptr);

        auto on_exit = [](uv_process_t *, int64_t exit_status, int signal) {
            Logger().info("Exit: status={} signal={}", exit_status, signal);
            exit(exit_status);
        };

        uv_process_options_t options{};
        options.exit_cb = on_exit;
        options.file = nvim.data();
        options.args = args.data();
#ifdef _WIN32
        // May be undefined in older versions of Ubuntu
        options.flags = UV_PROCESS_WINDOWS_HIDE | UV_PROCESS_WINDOWS_HIDE_CONSOLE;
#endif //_WIN32

        uv_pipe_t stdin_pipe, stdout_pipe;
        uv_pipe_init(loop, &stdin_pipe, 0);
        uv_pipe_init(loop, &stdout_pipe, 0);

        uv_stdio_container_t child_stdio[3];
        child_stdio[0].flags = static_cast<uv_stdio_flags>(UV_CREATE_PIPE | UV_READABLE_PIPE);
        child_stdio[0].data.stream = reinterpret_cast<uv_stream_t*>(&stdin_pipe);
        child_stdio[1].flags = static_cast<uv_stdio_flags>(UV_CREATE_PIPE | UV_WRITABLE_PIPE);
        child_stdio[1].data.stream = reinterpret_cast<uv_stream_t*>(&stdout_pipe);
        child_stdio[2].flags = UV_INHERIT_FD;
        child_stdio[2].data.fd = 2;
        options.stdio_count = 3;
        options.stdio = child_stdio;

        uv_process_t child_req;
        if (int r = ::uv_spawn(loop, &child_req, &options))
        {
            Logger().error("Failed to spawn: {}", uv_strerror(r));
            return 1;
        }

        MsgPackRpc rpc(&stdin_pipe, &stdout_pipe);

        std::unique_ptr<App> app;
        Timer delay(loop);
        // This is a race technically. If neovim doesn't start in time,
        // the UI will begin attaching. This may break handling of `--help`.
        delay.Start(100, 0, [&] {
            app.reset(new App(loop, rpc));
        });

        ::uv_run(loop, UV_RUN_DEFAULT);
    }
    catch (std::exception& e)
    {
        Logger().error("Exception: {}", e.what());
    }

    return 0;
}
