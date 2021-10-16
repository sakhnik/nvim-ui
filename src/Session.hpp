#pragma once

#include "MsgPackRpc.hpp"
#include "Renderer.hpp"
#include "RedrawHandler.hpp"
#include "Input.hpp"
#include <memory>
#include <atomic>
#include <thread>
#include <uv.h>

struct IWindow;

class Session
{
public:
    using PtrT = std::unique_ptr<Session>;

    virtual ~Session();

    void RunAsync();
    virtual void SetWindow(IWindow *);

    Renderer* GetRenderer() { return _renderer.get(); }
    Input* GetInput() { return _input.get(); }

    bool IsRunning() const
    {
        return !_nvim_exited.load(std::memory_order_relaxed);
    }

    const std::string& GetOutput() const
    {
        return _rpc->GetOutput();
    }

protected:
    std::unique_ptr<MsgPackRpc> _rpc;
    std::unique_ptr<Renderer> _renderer;
    std::unique_ptr<RedrawHandler> _redraw_handler;
    std::unique_ptr<Input> _input;

    std::unique_ptr<std::thread> _thread;
    std::atomic<bool> _nvim_exited = false;
    IWindow *_window = nullptr;

    void _Init(uv_stream_t *in, uv_stream_t *out);
    void _OnError(const char *);
    void _Exit();
};
