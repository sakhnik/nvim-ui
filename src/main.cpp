#include "MsgPackRpc.hpp"
#include "Renderer.hpp"
#include "RedrawHandler.hpp"
#include "Window.hpp"
#include "Logger.hpp"
#include "Timer.hpp"
#include "Input.hpp"
#include <spdlog/cfg/env.h>
#include <iostream>
#include <thread>

#include <uv.h>

int main(int argc, char* argv[])
{
    setlocale(LC_CTYPE, "");
    spdlog::cfg::load_env_levels();

    Logger().info("nvim-ui v{}", VERSION);
    try
    {
        std::string nvim("nvim");
        std::string embed("--embed");

        std::vector<char *> args;
        args.push_back(nvim.data());
        args.push_back(embed.data());
        for (int i = 1; i < argc; ++i)
            args.push_back(argv[i]);
        args.push_back(nullptr);

        struct Context
        {
            std::atomic<bool> nvim_exited = false;
        } ctx;

        auto loop = uv_default_loop();

        //auto signal = loop->resource<uvw::SignalHandle>();
        //signal->once<uvw::SignalEvent>([&loop](const uvw::SignalEvent &ev, const auto &handle) {
        //    if (ev.signum == SIGINT || ev.signum == SIGTERM)
        //    {
        //        loop->close();
        //    }
        //});

        auto on_exit = [](uv_process_t *proc, int64_t exit_status, int signal) {
            Context *ctx = reinterpret_cast<Context *>(proc->data);
            Logger().info("Exit: status={} signal={}", exit_status, signal);
            ctx->nvim_exited.store(true, std::memory_order_relaxed);
            uv_stop(uv_default_loop());
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
        child_req.data = &ctx;
        if (int r = ::uv_spawn(loop, &child_req, &options))
        {
            Logger().error("Failed to spawn: {}", uv_strerror(r));
            return 1;
        }

        MsgPackRpc rpc(&stdin_pipe, &stdout_pipe);
        Timer timer{loop};
        Renderer renderer{&rpc, &timer};
        RedrawHandler redraw_handler{&rpc, &renderer};

        if (!rpc.Activate())
            return 1;
        redraw_handler.AttachUI();

        Input input(loop, &rpc);
        Window window{&renderer, &input};


        std::thread t([&] {
            try
            {
                ::uv_run(loop, UV_RUN_DEFAULT);
            }
            catch (std::exception &e)
            {
                Logger().critical("Exception: {}", e.what());
            }
            catch (...)
            {
                Logger().critical("Unknown exception");
            }
        });

        while (!ctx.nvim_exited.load(std::memory_order_relaxed))
        {
            g_main_context_iteration(g_main_context_default(), true);
        }

        t.join();
        Logger().info("After join");
    }
    catch (std::exception& e)
    {
        Logger().error("Exception: {}", e.what());
    }

    return 0;
}
