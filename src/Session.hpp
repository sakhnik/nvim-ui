#pragma once

#include "MsgPackRpc.hpp"
#include "Timer.hpp"
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
    Session(int argc, char *argv[]);
    ~Session();

    void RunAsync(IWindow *);

    Renderer* GetRenderer() { return _renderer.get(); }
    Input* GetInput() { return _input.get(); }

    bool IsRunning() const
    {
        return !_nvim_exited.load(std::memory_order_relaxed);
    }

private:
    uv_pipe_t _stdin_pipe, _stdout_pipe;
    uv_process_t _child_req;
    std::unique_ptr<MsgPackRpc> _rpc;
    std::unique_ptr<Timer> _timer;
    std::unique_ptr<Renderer> _renderer;
    std::unique_ptr<RedrawHandler> _redraw_handler;
    std::unique_ptr<Input> _input;

    std::unique_ptr<std::thread> _thread;
    std::atomic<bool> _nvim_exited = false;
    IWindow *_window = nullptr;
};
