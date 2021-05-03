#pragma once

#include <string>
#include <mutex>
#include <uv.h>

class MsgPackRpc;

class Input
{
public:
    Input(uv_loop_t *loop, MsgPackRpc *);

    // Feed input keys
    void Accept(std::string_view input);

private:
    MsgPackRpc *_rpc;
    uv_async_t _available;
    std::string _input;
    std::mutex _mutex;

    static void _OnInput(uv_async_t *);
    void _OnInput2();
};
