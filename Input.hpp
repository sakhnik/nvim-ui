#pragma once

#include <string>
#include <uv.h>

class MsgPackRpc;
class Renderer;

class Input
{
public:
    Input(uv_loop_t *, MsgPackRpc *, Renderer *);
    ~Input();

    void Start();

private:
    MsgPackRpc *_rpc;
    Renderer *_renderer;
    uv_timer_t _timer;
    bool _shift = false;
    bool _control = false;

    void _PollEvents();
    void _OnInput(std::string_view input);
    void _RawInput(const char *input);
};
