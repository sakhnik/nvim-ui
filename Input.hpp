#pragma once

#include <string>
#include <uv.h>

class MsgPackRpc;

class Input
{
public:
    Input(uv_loop_t *, MsgPackRpc *);
    ~Input();

    void Start();

private:
    MsgPackRpc *_rpc;
    uv_timer_t _timer;
    bool _shift = false;
    bool _control = false;

    void _PollEvents();
    void _OnInput(const std::string &input);
    void _RawInput(const char *input);
};
