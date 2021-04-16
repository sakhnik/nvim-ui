#pragma once

#include "Timer.hpp"
#include <string_view>
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
    Timer _timer;
    bool _shift = false;
    bool _control = false;

    void _StartTimer();
    void _PollEvents();
    void _OnInput(std::string_view input);
    void _RawInput(const char *input);
};
