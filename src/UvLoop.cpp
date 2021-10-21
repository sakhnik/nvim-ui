#include "UvLoop.hpp"
#include "Logger.hpp"

UvLoop::UvLoop()
{
    if (int err = uv_loop_init(&_loop))
        throw std::runtime_error(fmt::format("Failed to init uv loop: {}", uv_strerror(err)));
}

UvLoop::~UvLoop()
{
    uv_stop(&_loop);
    if (_thread && _thread->joinable())
        _thread->join();
    if (int err = uv_loop_close(&_loop))
        Logger().warn("Failed to close loop: {}", uv_strerror(err));
}

void UvLoop::RunAsync()
{
    _thread.reset(new std::thread([&] {
        try
        {
            if (int err = ::uv_run(&_loop, UV_RUN_DEFAULT))
                Logger().error("Failed to run the loop: {}", uv_strerror(err));
        }
        catch (std::exception &e)
        {
            Logger().critical("Exception: {}", e.what());
        }
        catch (...)
        {
            Logger().critical("Unknown exception");
        }
    }));
}
