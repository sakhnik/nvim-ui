#include "SessionTcp.hpp"
#include "Logger.hpp"
#include <netinet/in.h>

SessionTcp::SessionTcp(const char *addr, int port)
{
    if (int err = uv_tcp_init(&_loop, &_socket))
        throw std::runtime_error(fmt::format("Failed to init tcp: {}", uv_strerror(err)));
    if (int err = uv_tcp_nodelay(&_socket, 1))
        Logger().warn(fmt::format("Failed to set tcp nodelay: {}", uv_strerror(err)));

    sockaddr_in req_addr;
    if (int err = uv_ip4_addr(addr, port, &req_addr))
        throw std::runtime_error(fmt::format("Failed to parse the IP address: {}", uv_strerror(err)));

    auto on_connect = [](uv_connect_t *req, int status) {
        Logger().info("Connected");
        if (status < 0) {
            Logger().error("Couldn't connect: {}", uv_strerror(status));
            return;
        }
        SessionTcp *session = reinterpret_cast<SessionTcp*>(req->data);
        session->_Init(req->handle, req->handle);
        session->_renderer->SetWindow(session->_window);
    };

    _connect.data = this;
    if (int err = uv_tcp_connect(&_connect, &_socket, reinterpret_cast<const sockaddr*>(&req_addr), on_connect))
        throw std::runtime_error(fmt::format("Failed to connect: {}", uv_strerror(err)));
}

void SessionTcp::SetWindow(IWindow *window)
{
    _window = window;
    // The window will be passed to the renderer upon successful connection
}
