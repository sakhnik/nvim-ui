#pragma once

#include <gio/gio.h>

class TcpServer
{
public:
    TcpServer();
    ~TcpServer();

private:
    GSocketService *_service;
    guint16 _port;
};
