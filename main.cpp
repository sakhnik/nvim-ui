#include <iostream>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/write.hpp>
#include <msgpack.hpp>

namespace bio = boost::asio;

void CheckError(const boost::system::error_code &err, size_t n)
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
            CheckError(err, n);
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
            CheckError(err, n);
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
};


void attach_ui(MsgPackRpc *rpc)
{
    rpc->Request(
        [](auto &pk) {
            pk.pack(std::string{"nvim_ui_attach"});
            pk.pack_array(3);
            pk.pack(80);
            pk.pack(25);
            pk.pack_map(0);
        },
        [](const msgpack::object &err, const auto &resp) {
            if (!err.is_nil())
            {
                std::ostringstream oss;
                oss << "Failed to attach UI " << err << std::endl;
                throw std::runtime_error(oss.str());
            }
        }
    );
}

int main(int argc, char* argv[])
{
    try
    {
        bio::io_context io_context(1);

        bio::signal_set signals(io_context, SIGINT, SIGTERM);
        signals.async_wait([&](auto, auto){ io_context.stop(); });

        bio::ip::tcp::socket socket{io_context};
        socket.connect({bio::ip::address::from_string("127.0.0.1"), 4444});

        std::unique_ptr<MsgPackRpc> rpc{new MsgPackRpc{io_context, socket}};
        rpc->OnNotifications(
            [] (std::string method, const msgpack::object &) {
                std::cout << "Notification " << method << std::endl;
            }
        );

        attach_ui(rpc.get());

        io_context.run();
    }
    catch (std::exception& e)
    {
        std::cout << "Exception: " << e.what() << std::endl;
    }

    return 0;
}
