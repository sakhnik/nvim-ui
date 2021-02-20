#pragma once

#include <functional>
#include <string>
#include <msgpack.hpp>
#include <uv.h>
#include <iostream>


struct MsgPackRpc
{
    virtual ~MsgPackRpc()
    {
    }

    using OnNotificationT = std::function<void(std::string, msgpack::object)>;

    void OnNotifications(OnNotificationT on_notification)
    {
        _on_notification = on_notification;
    }

    using PackerT = msgpack::packer<msgpack::sbuffer>;
    using PackRequestT = std::function<void(PackerT &)>;
    using OnResponseT = std::function<void(const msgpack::object &err, const msgpack::object &resp)>;

    virtual void Request(PackRequestT pack_request, OnResponseT on_response) = 0;

protected:
    OnNotificationT _on_notification;
};

class MsgPackRpcImpl : public MsgPackRpc
{
public:
    MsgPackRpcImpl(uv_pipe_t *stdin_pipe,
                   uv_pipe_t *stdout_pipe)
        : _stdin_pipe{stdin_pipe}
        , _stdout_pipe{stdout_pipe}
    {
        _stdout_pipe->data = this;

        auto alloc_buffer = [](uv_handle_t *handle, size_t len, uv_buf_t *buf) {
            MsgPackRpcImpl *self = reinterpret_cast<MsgPackRpcImpl*>(handle->data);
            self->_unp.reserve_buffer(len);
            buf->base = self->_unp.buffer();
            buf->len = len;
        };

        auto read_apipe = [](uv_stream_t* stream, ssize_t nread, const uv_buf_t *buf) {
            //printf("read %li bytes in a %lu byte buffer\n", nread, buf.len);
            MsgPackRpcImpl *self = reinterpret_cast<MsgPackRpcImpl*>(stream->data);
            self->_handle_data(buf->base, buf->len);
        };

        ::uv_read_start((uv_stream_t*)_stdout_pipe, alloc_buffer, read_apipe);
    }

    void Request(PackRequestT pack_request, OnResponseT on_response) override
    {
        auto [it, _] = _requests.emplace(_seq++, std::move(on_response));

        struct Write : uv_write_t
        {
            msgpack::sbuffer buffer;
        };
        std::unique_ptr<Write> w{new Write};

        // serializes multiple objects using msgpack::packer.
        msgpack::packer<msgpack::sbuffer> pk(&w->buffer);
        pk.pack_array(4);
        pk.pack(0);
        pk.pack(it->first);
        pack_request(pk);


        auto cb = [](uv_write_t* req, int status) {
            // TODO: check status
            delete reinterpret_cast<Write*>(req);
        };

        uv_buf_t buf = {
            .base = w->buffer.data(),
            .len = w->buffer.size(),
        };

        uv_write(w.release(), reinterpret_cast<uv_stream_t*>(_stdin_pipe), &buf, 1, cb);
    }

private:
    uv_pipe_t *_stdin_pipe;
    uv_pipe_t *_stdout_pipe;
    msgpack::unpacker _unp;
    uint32_t _seq = 0;
    std::map<uint32_t, OnResponseT> _requests;

    void _handle_data(const char *data, size_t length)
    {
        _unp.buffer_consumed(length);

        msgpack::unpacked result;
        while (_unp.next(result))
        {
            msgpack::object obj{result.get()};
            const auto &arr = obj.via.array;
            if (arr.ptr[0] == 1)
            {
                // Response
                auto it = _requests.find(arr.ptr[1].as<uint32_t>());
                it->second(arr.ptr[2], arr.ptr[3]);
                _requests.erase(it);
            }
            else if (arr.ptr[0] == 2)
            {
                // Notification
                _on_notification(arr.ptr[1].as<std::string>(), arr.ptr[2]);
            }
        }
    }
};
