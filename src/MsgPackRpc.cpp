#include "MsgPackRpc.hpp"
#include "Logger.hpp"
#include <iostream>


MsgPackRpc::MsgPackRpc(uv_pipe_t *stdin_pipe, uv_pipe_t *stdout_pipe)
    : _stdin_pipe{stdin_pipe}
    , _stdout_pipe{stdout_pipe}
{
    _stdout_pipe->data = this;

    auto alloc_buffer = [](uv_handle_t *handle, size_t len, uv_buf_t *buf) {
        MsgPackRpc *self = reinterpret_cast<MsgPackRpc*>(handle->data);
        self->_unp.reserve_buffer(len);
        buf->base = self->_unp.buffer();
        buf->len = len;
    };

    auto read_apipe = [](uv_stream_t* stream, ssize_t nread, const uv_buf_t *buf) {
        if (nread > 0)
        {
            MsgPackRpc *self = reinterpret_cast<MsgPackRpc*>(stream->data);
            self->_handle_data(buf->base, nread);
        }
    };

    ::uv_read_start((uv_stream_t*)_stdout_pipe, alloc_buffer, read_apipe);
}

MsgPackRpc::~MsgPackRpc()
{
    ::uv_read_stop((uv_stream_t*)_stdout_pipe);
}

void MsgPackRpc::Request(PackRequestT pack_request, OnResponseT on_response)
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
        Write *w = reinterpret_cast<Write *>(req);
        if (status < 0)
            Logger().error("Failed to write {} bytes: {}", w->buffer.size(), status);
        delete w;
    };

    uv_buf_t buf;
    buf.base = w->buffer.data();
    buf.len = w->buffer.size();

    uv_write(w.release(), reinterpret_cast<uv_stream_t*>(_stdin_pipe), &buf, 1, cb);
}

void MsgPackRpc::_handle_data(const char *data, size_t length)
{
    _unp.buffer_consumed(length);

    msgpack::unpacked result;
    while (_unp.next(result))
    {
        msgpack::object obj{result.get()};
        // It's unlikely that this check will be passed unless
        // msgpack RPC is established.
        if (obj.type != msgpack::type::ARRAY)
        {
            // Bail out to capture output of --help, --version etc.
            _output.append(std::string_view(data, length));
            return;
        }
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
            _on_notification(arr.ptr[1].as<std::string_view>(), arr.ptr[2]);
        }
    }
}
