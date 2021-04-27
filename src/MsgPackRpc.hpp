#pragma once

#include <functional>
#include <string>
#include <msgpack.hpp>
#include <uv.h>


class MsgPackRpc
{
public:
    MsgPackRpc(uv_pipe_t *stdin_pipe, uv_pipe_t *stdout_pipe);

    using OnNotificationT = std::function<void(std::string_view, msgpack::object)>;

    void OnNotifications(OnNotificationT on_notification)
    {
        _on_notification = on_notification;
    }

    using PackerT = msgpack::packer<msgpack::sbuffer>;
    using PackRequestT = std::function<void(PackerT &)>;
    using OnResponseT = std::function<void(const msgpack::object &err, const msgpack::object &resp)>;

    void Request(PackRequestT pack_request, OnResponseT on_response);
    bool Activate();

private:
    OnNotificationT _on_notification;
    uv_pipe_t *_stdin_pipe;
    uv_pipe_t *_stdout_pipe;
    // If any output is captured before activation, prevent activation.
    bool _dirty = false;
    bool _activated = false;
    msgpack::unpacker _unp;
    uint32_t _seq = 0;
    std::map<uint32_t, OnResponseT> _requests;

    void _handle_data(const char *data, size_t length);
};
