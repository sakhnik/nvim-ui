#include "SessionSpawn.hpp"
#include "Logger.hpp"

SessionSpawn::SessionSpawn(int argc, char *argv[])
{
    std::string nvim("nvim");
    std::string embed("--embed");

    std::vector<char *> args;
    args.push_back(nvim.data());
    args.push_back(embed.data());
    for (int i = 1; i < argc; ++i)
        args.push_back(argv[i]);
    args.push_back(nullptr);

    auto loop = uv_default_loop();

    //auto signal = loop->resource<uvw::SignalHandle>();
    //signal->once<uvw::SignalEvent>([&loop](const uvw::SignalEvent &ev, const auto &handle) {
    //    if (ev.signum == SIGINT || ev.signum == SIGTERM)
    //    {
    //        loop->close();
    //    }
    //});

    auto on_exit = [](uv_process_t *proc, int64_t exit_status, int signal) {
        Logger().info("Exit: status={} signal={}", exit_status, signal);
        SessionSpawn *session = reinterpret_cast<SessionSpawn *>(proc->data);
        session->_Exit();
    };

    uv_process_options_t options{};
    options.exit_cb = on_exit;
    options.file = nvim.data();
    options.args = args.data();
#ifdef _WIN32
    // May be undefined in older versions of Ubuntu
    options.flags = UV_PROCESS_WINDOWS_HIDE | UV_PROCESS_WINDOWS_HIDE_CONSOLE;
#endif //_WIN32

    uv_pipe_init(loop, &_stdin_pipe, 0);
    uv_pipe_init(loop, &_stdout_pipe, 0);

    uv_stdio_container_t child_stdio[3];
    child_stdio[0].flags = static_cast<uv_stdio_flags>(UV_CREATE_PIPE | UV_READABLE_PIPE);
    child_stdio[0].data.stream = reinterpret_cast<uv_stream_t*>(&_stdin_pipe);
    child_stdio[1].flags = static_cast<uv_stdio_flags>(UV_CREATE_PIPE | UV_WRITABLE_PIPE);
    child_stdio[1].data.stream = reinterpret_cast<uv_stream_t*>(&_stdout_pipe);
    child_stdio[2].flags = UV_INHERIT_FD;
    child_stdio[2].data.fd = 2;
    options.stdio_count = 3;
    options.stdio = child_stdio;

    _child_req.data = this;
    if (int r = ::uv_spawn(loop, &_child_req, &options))
    {
        std::string message = "Failed to spawn: ";
        message += uv_strerror(r);
        Logger().error(message.c_str());
        throw std::runtime_error(message.c_str());
    }

    _Init(reinterpret_cast<uv_stream_t*>(&_stdin_pipe), reinterpret_cast<uv_stream_t*>(&_stdout_pipe));
}
