#include "MsgPackRpc.hpp"


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
        //printf("read %li bytes in a %lu byte buffer\n", nread, buf.len);
        if (nread > 0)
        {
            MsgPackRpc *self = reinterpret_cast<MsgPackRpc*>(stream->data);
            self->_handle_data(buf->base, nread);
        }
    };

    ::uv_read_start((uv_stream_t*)_stdout_pipe, alloc_buffer, read_apipe);
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
        // TODO: check status
        delete reinterpret_cast<Write*>(req);
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
