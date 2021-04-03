#pragma once

#include "MsgPackRpc.hpp"
#include <sstream>
#include <termkey.h>
#include <uv.h>
#include <unistd.h>

class Input
{
public:
    Input(uv_loop_t *loop, MsgPackRpc *rpc)
        : _rpc{rpc}
        //, _descriptor{io_context}
        //, _timer{io_context}
    {
        _tk = ::termkey_new(0, 0);
        if (!_tk)
            throw std::runtime_error("Cannot allocate termkey instance");

        ::uv_poll_init(loop, &_poll, STDIN_FILENO);
        _poll.data = this;

        ::uv_timer_init(loop, &_timer);
        _timer.data = this;
    }

    ~Input()
    {
        ::termkey_destroy(_tk);
    }

    void Start()
    {
        auto on_readable = [](uv_poll_t *handle, int status, int events) {
            if (status == UV_EAGAIN)
                return;
            Input *self = reinterpret_cast<Input*>(handle->data);
            self->_OnReadable();
        };

        ::uv_poll_start(&_poll, UV_READABLE, on_readable);

        _StartTimer(-1);
    }

private:
    MsgPackRpc *_rpc;
    uv_poll_t _poll;
    uv_timer_t _timer;
    TermKey *_tk;

    void _StartTimer(int nextwait)
    {
        auto on_timeout = [](uv_timer_t *handle) {
            Input *self = reinterpret_cast<Input*>(handle->data);
            TermKeyKey key;
            if (::termkey_getkey_force(self->_tk, &key) == TERMKEY_RES_KEY)
                self->_OnKey(key);
        };

        if (nextwait != -1)
            ::uv_timer_start(&_timer, on_timeout, nextwait, 0);
        else
            ::uv_timer_stop(&_timer);
    }

    void _OnKey(TermKeyKey &key)
    {
        // see https://github.com/neovim/neovim/blob/master/src/nvim/tui/input.c for these remappings
        std::string input;
        if (key.type == TERMKEY_TYPE_UNICODE && key.code.codepoint == '<')
            input = "<lt>";
        else if (key.type == TERMKEY_TYPE_KEYSYM && key.code.codepoint == TERMKEY_SYM_BACKSPACE)
            input = "<bs>";
        else if (key.type == TERMKEY_TYPE_KEYSYM && key.code.codepoint == TERMKEY_SYM_ESCAPE)
            input = "<esc>";
        else
        {
            char buffer[50];
            ::termkey_strfkey(_tk, buffer, sizeof(buffer), &key, TERMKEY_FORMAT_VIM);
            input = buffer;
        }
        std::cerr << "Input " << input << std::endl;

        _rpc->Request(
            [&](MsgPackRpc::PackerT &pk) {
                pk.pack(std::string{"nvim_input"});
                pk.pack_array(1);
                pk.pack(input);
            },
            [](const msgpack::object &err, const auto &resp) {
                if (!err.is_nil())
                {
                    std::ostringstream oss;
                    oss << "Input error: " << err;
                    throw std::runtime_error(oss.str());
                }
            }
        );
    }

    void _OnReadable()
    {
        ::termkey_advisereadable(_tk);

        TermKeyResult ret;
        TermKeyKey key;

        while ((ret = termkey_getkey(_tk, &key)) == TERMKEY_RES_KEY)
            _OnKey(key);

        _StartTimer((ret == TERMKEY_RES_AGAIN) ? termkey_get_waittime(_tk) : -1);
    }
};
