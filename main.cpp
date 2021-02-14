#include "MsgPackRpc.hpp"

#include <iostream>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/signal_set.hpp>
#include <msgpack.hpp>

namespace bio = boost::asio;


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
