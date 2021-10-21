#include "Input.hpp"
#include "MsgPackRpc.hpp"
#include "Logger.hpp"


Input::Input(uv_loop_t *loop, MsgPackRpc *rpc)
    : _rpc{rpc}
{
    uv_async_init(loop, &_available, _OnInput);
    _available.data = this;
}

Input::~Input()
{
    auto nop = [](uv_handle_t *) { };
    uv_close(reinterpret_cast<uv_handle_t *>(&_available), nop);
}

void Input::_OnInput(uv_async_t *a)
{
    reinterpret_cast<Input *>(a->data)->_OnInput2();
}

void Input::_OnInput2()
{
    std::lock_guard<std::mutex> guard{_mutex};
    std::string input{std::move(_input)};
    size_t input_size = input.size();
    _rpc->Request(
        [&](MsgPackRpc::PackerT &pk) {
            pk.pack("nvim_input");
            pk.pack_array(1);
            pk.pack(input);
        },
        [input_size](const msgpack::object &err, const msgpack::object &resp) {
            if (!err.is_nil())
            {
                std::ostringstream oss;
                oss << "Input error: " << err;
                throw std::runtime_error(oss.str());
            }
            size_t consumed = resp.as<size_t>();
            if (consumed < input_size)
                Logger().warn("[input] Consumed {}/{} bytes", consumed, input_size);
        }
    );
}

void Input::Accept(std::string_view input)
{
    {
        std::lock_guard<std::mutex> guard{_mutex};
        _input += input;
    }
    uv_async_send(&_available);
}
