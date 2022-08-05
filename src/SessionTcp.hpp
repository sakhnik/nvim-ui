#pragma once

#include "Session.hpp"
#include <uv.h>

class SessionTcp : public Session
{
public:
    SessionTcp(const char *addr, int port);
    ~SessionTcp() override;

    void SetWindow(IWindow *) override;

    const std::string& GetDescription() const override;

private:
    std::string _description;
    std::unique_ptr<uv_tcp_t> _socket;
    uv_connect_t _connect;
};
