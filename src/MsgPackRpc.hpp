#pragma once

#include <functional>
#include <string>
#include <msgpack.hpp>
#include <uv.h>


class MsgPackRpc
{
public:
    MsgPackRpc(uv_stream_t *, uv_stream_t *);
    ~MsgPackRpc();

    using OnNotificationT = std::function<void(std::string_view, msgpack::object)>;

    void OnNotifications(OnNotificationT on_notification)
    {
        _on_notification = on_notification;
    }

    using PackerT = msgpack::packer<msgpack::sbuffer>;
    using PackRequestT = std::function<void(PackerT &)>;
    using OnResponseT = std::function<void(const msgpack::object &err, const msgpack::object &resp)>;

    void Request(PackRequestT, OnResponseT);
    void RequestAsync(PackRequestT pack_request, OnResponseT);

    const std::string& GetOutput() const { return _output; }

private:
    uv_stream_t *_stdin_stream;
    uv_stream_t *_stdout_stream;
    OnNotificationT _on_notification;

    // Capture any non msgpack-rpc output, as it may be text output from --version or alike
    std::string _output;

    msgpack::unpacker _unp;
    uint32_t _seq = 0;
    std::map<uint32_t, OnResponseT> _requests;

    void _handle_data(const char *data, size_t length);
};
