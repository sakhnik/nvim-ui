#include "MsgPackRpc.hpp"
#include "Renderer.hpp"
#include "RedrawHandler.hpp"
#include "Window.hpp"
#include "Input.hpp"
#include "Logger.hpp"
#include <spdlog/cfg/env.h>

#include <uv.h>


int main(int argc, char* argv[])
{
    setlocale(LC_CTYPE, "");
    spdlog::cfg::load_env_levels();

    Logger::I().info("nvim-ui v{}", VERSION);
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

        char *args[] = {
            nvim.data(),
            embed.data(),
            nullptr,
        };

        auto on_exit = [](uv_process_t *, int64_t exit_status, int signal) {
            exit(exit_status);
        };

        uv_process_options_t options{};
        options.exit_cb = on_exit;
        options.file = nvim.data();
        options.args = args;
        options.flags = UV_PROCESS_WINDOWS_HIDE | UV_PROCESS_WINDOWS_HIDE_CONSOLE;

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
            Logger::I().error("Failed to spawn: {}", uv_strerror(r));
            return 1;
        }

        MsgPackRpc rpc(&stdin_pipe, &stdout_pipe);
        Window window;
        Timer timer{loop};
        Renderer renderer(&rpc, &window, &timer);
        RedrawHandler redraw_handler(&rpc, &renderer);
        redraw_handler.AttachUI();

        Input input{loop, &rpc, &renderer};
        input.Start();

        ::uv_run(loop, UV_RUN_DEFAULT);
    }
    catch (std::exception& e)
    {
        Logger::I().error("Exception: {}", e.what());
    }

    return 0;
}
