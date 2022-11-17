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

    _hints.ai_family = PF_INET;
    _hints.ai_socktype = SOCK_STREAM;
    _hints.ai_protocol = IPPROTO_TCP;
    _hints.ai_flags = 0;

    sockaddr_in req_addr;
    int err = uv_ip4_addr(addr, port, &req_addr);
    if (!err)
    {
        _Connect(reinterpret_cast<const sockaddr*>(&req_addr));
        return;
    }
    _resolver.data = this;

    auto on_resolved = [](uv_getaddrinfo_t *resolver, int status, struct addrinfo *res) {
        SessionTcp *session = reinterpret_cast<SessionTcp*>(resolver->data);
        if (status < 0) {
            session->_window->SetError(uv_strerror(status));
            session->_window->SessionEnd();
            Logger().error(fmt::format("Failed to resolve the address: {}", uv_strerror(status)));
            return;
        }

        char addr[17] = {'\0'};
        uv_ip4_name(reinterpret_cast<const sockaddr_in*>(res->ai_addr), addr, 16);
        Logger().debug(fmt::format("Resolved to {}", addr));

        session->_Connect(res->ai_addr);

        uv_freeaddrinfo(res);
    }; 

    err = uv_getaddrinfo(&_loop, &_resolver, on_resolved, addr, std::to_string(port).c_str(), &_hints);
    if (err)
        throw std::runtime_error(fmt::format("Failed to resolve the address: {}", uv_strerror(err)));
}

void SessionTcp::_Connect(const sockaddr *addr)
{
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
    if (int err = uv_tcp_connect(&_connect, _socket.get(), addr, on_connect))
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
