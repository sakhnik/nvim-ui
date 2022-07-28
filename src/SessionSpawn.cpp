#include "SessionSpawn.hpp"
#include "Logger.hpp"

SessionSpawn::SessionSpawn(int argc, char *argv[])
    : _stdin_pipe{new uv_pipe_t}
    , _stdout_pipe{new uv_pipe_t}
    , _child_req{new uv_process_t}
{
    std::string nvim("nvim");
    std::string embed("--embed");

    const char *space = "";
    std::vector<char *> args;
    args.push_back(nvim.data());
    args.push_back(embed.data());
    for (int i = 1; i < argc; ++i)
    {
        args.push_back(argv[i]);
        _description += space;
        _description += argv[i];
        space = " ";
    }
    args.push_back(nullptr);

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

    if (int err = uv_pipe_init(&_loop, _stdin_pipe.get(), 0))
        throw std::runtime_error(fmt::format("Failed to init pipe: {}", uv_strerror(err)));
    if (int err = uv_pipe_init(&_loop, _stdout_pipe.get(), 0))
        throw std::runtime_error(fmt::format("Failed to init pipe: {}", uv_strerror(err)));

    uv_stdio_container_t child_stdio[3];
    child_stdio[0].flags = static_cast<uv_stdio_flags>(UV_CREATE_PIPE | UV_READABLE_PIPE);
    child_stdio[0].data.stream = reinterpret_cast<uv_stream_t*>(_stdin_pipe.get());
    child_stdio[1].flags = static_cast<uv_stdio_flags>(UV_CREATE_PIPE | UV_WRITABLE_PIPE);
    child_stdio[1].data.stream = reinterpret_cast<uv_stream_t*>(_stdout_pipe.get());
    child_stdio[2].flags = UV_INHERIT_FD;
    child_stdio[2].data.fd = 2;
    options.stdio_count = 3;
    options.stdio = child_stdio;

    _child_req->data = this;
    if (int err = ::uv_spawn(&_loop, _child_req.get(), &options))
        throw std::runtime_error(fmt::format("Failed to spawn: {}", uv_strerror(err)));

    _Init(reinterpret_cast<uv_stream_t*>(_stdin_pipe.get()), reinterpret_cast<uv_stream_t*>(_stdout_pipe.get()));
}

SessionSpawn::~SessionSpawn()
{
    auto nop = [](uv_handle_t *h) {
        delete h;
    };
    uv_close(reinterpret_cast<uv_handle_t*>(_stdin_pipe.release()), nop);
    uv_close(reinterpret_cast<uv_handle_t*>(_stdout_pipe.release()), nop);
    uv_close(reinterpret_cast<uv_handle_t*>(_child_req.release()), nop);
}

const std::string& SessionSpawn::GetDescription() const
{
    return _description;
}
