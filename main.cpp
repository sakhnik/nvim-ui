#include "MsgPackRpc.hpp"
#include "Renderer.hpp"

#include <iostream>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/signal_set.hpp>
#include <msgpack.hpp>

namespace bio = boost::asio;


int main(int argc, char* argv[])
{
    setlocale(LC_CTYPE, "");
    try
    {
        bio::io_context io_context(1);

        bio::signal_set signals(io_context, SIGINT, SIGTERM);
        signals.async_wait([&](auto, auto){ io_context.stop(); });

        bio::ip::tcp::socket socket{io_context};
        socket.connect({bio::ip::address::from_string("127.0.0.1"), 4444});

        std::unique_ptr<MsgPackRpc> rpc{new MsgPackRpc{io_context, socket}};
        Renderer renderer(rpc.get());
        renderer.AttachUI();

        io_context.run();
    }
    catch (std::exception& e)
    {
        std::cout << "Exception: " << e.what() << std::endl;
    }

    return 0;
}
