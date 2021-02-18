#include "MsgPackRpc.hpp"
#include "Renderer.hpp"
#include "Input.hpp"

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

        MsgPackRpcImpl<bio::ip::tcp::socket, bio::ip::tcp::socket> rpc{io_context, socket, socket};
        Renderer renderer(&rpc);
        renderer.AttachUI();

        Input input{io_context, &rpc};
        input.Start();

        io_context.run();
    }
    catch (std::exception& e)
    {
        std::cout << "Exception: " << e.what() << std::endl;
    }

    return 0;
}
