#pragma once

#include "AsyncExec.hpp"
#include <string>
#include <mutex>
#include <uv.h>

class MsgPackRpc;

class Input
{
public:
    Input(uv_loop_t *, MsgPackRpc *);

    // Feed input keys
    void Accept(std::string_view input);

private:
    MsgPackRpc *_rpc;
    AsyncExec _available;
    std::string _input;
    std::mutex _mutex;

    void _OnInput();
};
