#include "SessionTcp.hpp"
#include "Logger.hpp"
#include <netinet/in.h>

SessionTcp::SessionTcp(const char *addr, int port)
{
    uv_tcp_init(&_loop, &_socket);
    uv_tcp_nodelay(&_socket, 1);

    sockaddr_in req_addr;
    uv_ip4_addr(addr, port, &req_addr);

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
    uv_tcp_connect(&_connect, &_socket, reinterpret_cast<const sockaddr*>(&req_addr), on_connect);
}

void SessionTcp::SetWindow(IWindow *window)
{
    _window = window;
    // The window will be passed to the renderer upon successful connection
}
