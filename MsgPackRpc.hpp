#pragma once

#include <functional>
#include <string>
#include <msgpack.hpp>
#include <boost/asio/write.hpp>
#include <boost/asio/ip/tcp.hpp>

namespace bio = boost::asio;

class MsgPackRpc
{
public:
    using OnNotificationT = std::function<void(std::string, msgpack::object)>;

    MsgPackRpc(bio::io_context &io_context, bio::ip::tcp::socket &socket)
        : _socket{socket}
    {
        _dispatch();
    }

    void OnNotifications(OnNotificationT on_notification)
    {
        _on_notification = on_notification;
    }

    using PackerT = msgpack::packer<msgpack::sbuffer>;
    using PackRequestT = std::function<void(PackerT &)>;
    using OnResponseT = std::function<void(const msgpack::object &err, const msgpack::object &resp)>;

    void Request(PackRequestT pack_request, OnResponseT on_response)
    {
        auto [it, _] = _requests.emplace(_seq++, std::move(on_response));

        // serializes multiple objects using msgpack::packer.
        msgpack::sbuffer buffer;
        msgpack::packer<msgpack::sbuffer> pk(&buffer);
        pk.pack_array(4);
        pk.pack(0);
        pk.pack(it->first);
        pack_request(pk);
        async_write(_socket, bio::buffer(buffer.data(), buffer.size()), [](const auto &err, size_t n) {
            _CheckError(err, n);
        });
    }

private:
    bio::ip::tcp::socket &_socket;
    OnNotificationT _on_notification;
    msgpack::unpacker _unp;
    uint32_t _seq = 0;
    std::map<uint32_t, OnResponseT> _requests;

    void _dispatch()
    {
        _unp.reserve_buffer(1024);
        _socket.async_read_some(bio::buffer(_unp.buffer(), 1024), [&](const auto &err, size_t n) {
            _CheckError(err, n);
            if (!n) throw std::runtime_error("EOF");

            _unp.buffer_consumed(n);
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
            _dispatch();
        });
    }

    static void _CheckError(const boost::system::error_code &err, size_t n)
    {
        if (err)
        {
            std::ostringstream oss;
            oss << err << std::endl;
            throw std::runtime_error(oss.str());
        }
        if (!n)
            throw std::runtime_error("EOF");
    }
};

