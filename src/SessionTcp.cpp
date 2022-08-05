#include "SessionTcp.hpp"
#include "Logger.hpp"
#include "IWindow.hpp"
#ifdef _WIN32
# include <ws2def.h>
#else //_WIN32
# include <netinet/in.h>
#endif //_WIN32

SessionTcp::SessionTcp(const char *addr, int port)
    : _socket{new uv_tcp_t}
{
    _description = fmt::format("TCP {}:{}", addr, port);

    if (int err = uv_tcp_init(&_loop, _socket.get()))
        throw std::runtime_error(fmt::format("Failed to init tcp: {}", uv_strerror(err)));
    if (int err = uv_tcp_nodelay(_socket.get(), 1))
        Logger().warn(fmt::format("Failed to set tcp nodelay: {}", uv_strerror(err)));

    sockaddr_in req_addr;
    if (int err = uv_ip4_addr(addr, port, &req_addr))
        throw std::runtime_error(fmt::format("Failed to parse the IP address: {}", uv_strerror(err)));

    auto on_connect = [](uv_connect_t *req, int status) {
        SessionTcp *session = reinterpret_cast<SessionTcp*>(req->data);
        if (status < 0) {
            Logger().error("Couldn't connect: {}", uv_strerror(status));
            session->_window->SetError(uv_strerror(status));
            session->_window->SessionEnd();
            return;
        }
        Logger().info("Connected");
        session->_Init(req->handle, req->handle);
        session->_renderer->SetWindow(session->_window);
    };

    _connect.data = this;
    if (int err = uv_tcp_connect(&_connect, _socket.get(), reinterpret_cast<const sockaddr*>(&req_addr), on_connect))
        throw std::runtime_error(fmt::format("Failed to connect: {}", uv_strerror(err)));
}

SessionTcp::~SessionTcp()
{
    uv_tcp_close_reset(_socket.release(), [](uv_handle_t *h) {
        delete reinterpret_cast<uv_tcp_t*>(h);
    });
}

void SessionTcp::SetWindow(IWindow *window)
{
    _window = window;
    // The window will be passed to the renderer upon successful connection
}

const std::string& SessionTcp::GetDescription() const
{
    return _description;
}
