#include <iostream>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/write.hpp>
#include <msgpack.hpp>

namespace bio = boost::asio;

bio::awaitable<void> handle_notifications(bio::ip::tcp::socket &socket)
{
    msgpack::unpacker unp;

    try
    {
        char data[1024];
        for (;;)
        {
            std::size_t n = co_await socket.async_read_some(bio::buffer(data), bio::use_awaitable);
            std::cout << "read " << n << std::endl;

            msgpack::unpacked result;
            while (unp.next(result))
            {
                msgpack::object obj{result.get()};
                std::cout << "type " << obj.type << std::endl;
            }
            //co_await async_write(socket, bio::buffer(data, n), bio::use_awaitable);
        }
    }
    catch (const std::exception &e)
    {
        std::cout << e.what() << std::endl;
    }
}


bio::awaitable<void> attach_ui(bio::ip::tcp::socket &socket)
{
    // serializes multiple objects using msgpack::packer.
    msgpack::sbuffer buffer;
    msgpack::packer<msgpack::sbuffer> pk(&buffer);
    pk.pack(0);
    pk.pack(uint32_t{0});
    pk.pack(std::string{"nvim_ui_attach"});
    pk.pack(80);
    pk.pack(25);
    pk.pack_map(0);
    co_await async_write(socket, bio::buffer(buffer.data(), buffer.size()), bio::use_awaitable);
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

        bio::co_spawn(io_context, handle_notifications(socket), bio::detached);
        bio::co_spawn(io_context, attach_ui(socket), bio::detached);


        io_context.run();
    }
    catch (std::exception& e)
    {
        std::cout << "Exception: " << e.what() << std::endl;
    }

    return 0;
}
