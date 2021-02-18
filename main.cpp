#include "MsgPackRpc.hpp"
#include "Renderer.hpp"
#include "Input.hpp"

#include <iostream>
#include <boost/process.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/signal_set.hpp>
#include <msgpack.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>


namespace bio = boost::asio;
namespace bp = boost::process;

int main(int argc, char* argv[])
{
    setlocale(LC_CTYPE, "");
    try
    {
        auto logger = spdlog::basic_logger_st("logger", "/tmp/n.log");
        logger->info("Hello");

        bio::io_context io_context(1);

        bio::signal_set signals(io_context, SIGINT, SIGTERM);
        signals.async_wait([&](auto, auto){ io_context.stop(); });

        bp::async_pipe apfrom(io_context);
        bp::async_pipe apto(io_context);
        bp::child c(bp::search_path("nvim"), "--embed", bp::std_out > apfrom, bp::std_in < apto, io_context);

        auto rpc = MakeMsgPackRpcImpl(io_context, apfrom, apto);
        Renderer renderer(&rpc);
        renderer.AttachUI();

        Input input{io_context, &rpc};
        input.Start();

        io_context.run();
        c.wait();
        return c.exit_code();
    }
    catch (std::exception& e)
    {
        std::cout << "Exception: " << e.what() << std::endl;
    }

    return 0;
}
