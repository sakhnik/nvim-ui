#include "Session.hpp"
#include "Logger.hpp"
#include "IWindow.hpp"
#include <string>
#include <vector>

void Session::_Init(uv_stream_t *in, uv_stream_t *out)
{
    _rpc.reset(new MsgPackRpc(in, out));
    _renderer.reset(new Renderer{uv_default_loop(), _rpc.get()});
    _redraw_handler.reset(new RedrawHandler{_rpc.get(), _renderer.get()});

    _redraw_handler->AttachUI();

    _input.reset(new Input{uv_default_loop(), _rpc.get()});
}

Session::~Session()
{
    uv_stop(uv_default_loop());
    if (_thread && _thread->joinable())
        _thread->join();
}

void Session::RunAsync()
{
    _thread.reset(new std::thread([&] {
        try
        {
            ::uv_run(uv_default_loop(), UV_RUN_DEFAULT);
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

void Session::SetWindow(IWindow *window)
{
    _window = window;
    _renderer->SetWindow(window);
}
