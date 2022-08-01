#pragma once

#include "UvLoop.hpp"
#include "MsgPackRpc.hpp"
#include "Renderer.hpp"
#include "RedrawHandler.hpp"
#include "Input.hpp"
#include <memory>
#include <atomic>
#include <uv.h>

struct IWindow;

class Session
    : public UvLoop
{
public:
    // The session management is going to be shared between the neovim and gtk threads
    using PtrT = std::shared_ptr<Session>;
    using AtomicPtrT = std::atomic<PtrT>;

    virtual ~Session() = default;

    virtual void SetWindow(IWindow *);
    virtual const std::string& GetDescription() const = 0;

    Renderer* GetRenderer() { return _renderer.get(); }
    Input* GetInput() { return _input.get(); }

    bool IsRunning() const
    {
        return !_nvim_exited.load(std::memory_order_relaxed);
    }

    const std::string& GetOutput() const
    {
        static const std::string NOTHING = "";
        return _rpc ? _rpc->GetOutput() : NOTHING;
    }

protected:
    std::unique_ptr<MsgPackRpc> _rpc;
    std::unique_ptr<Renderer> _renderer;
    std::unique_ptr<RedrawHandler> _redraw_handler;
    std::unique_ptr<Input> _input;

    std::atomic<bool> _nvim_exited = false;
    IWindow *_window = nullptr;

    Session() = default;
    void _Init(uv_stream_t *in, uv_stream_t *out);
    void _OnError(const char *);
    void _Exit();
};
