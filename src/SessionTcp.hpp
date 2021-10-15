#pragma once

#include "Session.hpp"
#include <uv.h>

class SessionTcp : public Session
{
public:
    SessionTcp(const char *addr, int port);
    void SetWindow(IWindow *) override;

private:
    uv_tcp_t _socket;
    uv_connect_t _connect;
};
