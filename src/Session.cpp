#include "Session.hpp"
#include "Logger.hpp"
#include "IWindow.hpp"
#include <string>
#include <vector>

Session::Session()
{
    uv_loop_init(&_loop);
}

void Session::_Init(uv_stream_t *in, uv_stream_t *out)
{
    auto onError = [this](const char *error) { _OnError(error); };
    _rpc.reset(new MsgPackRpc(in, out, onError));
    _renderer.reset(new Renderer{&_loop, _rpc.get()});
    _redraw_handler.reset(new RedrawHandler{_rpc.get(), _renderer.get()});

    _redraw_handler->AttachUI();

    _input.reset(new Input{&_loop, _rpc.get()});
}

Session::~Session()
{
    uv_stop(&_loop);
    if (_thread && _thread->joinable())
        _thread->join();
}

void Session::RunAsync()
{
    _thread.reset(new std::thread([&] {
        try
        {
            ::uv_run(&_loop, UV_RUN_DEFAULT);
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

void Session::_OnError(const char *error)
{
    _Exit();
    if (_window && error)
        _window->SetError(error);
}

void Session::_Exit()
{
    _nvim_exited.store(true, std::memory_order_relaxed);
    uv_stop(&_loop);
    if (_window)
        _window->SessionEnd();
}
