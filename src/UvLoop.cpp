#include "UvLoop.hpp"
#include "Logger.hpp"
#include "AsyncExec.hpp"

UvLoop::UvLoop()
{
    if (int err = uv_loop_init(&_loop))
        throw std::runtime_error(fmt::format("Failed to init uv loop: {}", uv_strerror(err)));
}

UvLoop::~UvLoop()
{
    uv_stop(&_loop);
    {
        // Make sure to cause some io for the loop to quit
        AsyncExec async{&_loop};
        async.Post([]{});
    }
    if (_thread && _thread->joinable())
        _thread->join();
    switch (int err = uv_loop_close(&_loop))
    {
    case 0:
        break;
    case UV_EBUSY:
        {
            Logger().warn("Can't close the busy loop");
            auto walk_cb = [](uv_handle_t *h, void *) {
                Logger().warn("Unclosed handle: {}", uv_handle_type_name(uv_handle_get_type(h)));
                auto on_close = [](uv_handle_t *) { };
                if (!uv_is_closing(h))
                    uv_close(h, on_close);
            };
            uv_walk(&_loop, walk_cb, this);
        }
        break;
    default:
        Logger().warn("Failed to close loop: {}", uv_strerror(err));
    }
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
