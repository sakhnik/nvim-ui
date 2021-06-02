#pragma once

#include <gio/gio.h>

class TcpServer
{
public:
    TcpServer();
    ~TcpServer();

    guint16 GetPort() const { return _port; }

private:
    GSocketService *_service;
    guint16 _port;

    struct ConnData
    {
        ConnData(TcpServer *tcp_server, GSocketConnection *c)
            : tcp_server{tcp_server}
            , connection{g_object_ref(c)}
        {
        }

        ~ConnData()
        {
            g_object_unref(connection);
        }

        TcpServer *tcp_server;
        GSocketConnection *connection;
        char message[1024];
    };

    void _OnConnection(GSocketConnection *);
    void _ReceiveMessage(ConnData *);
    void _MessageReady(GAsyncResult *, ConnData *);
};
