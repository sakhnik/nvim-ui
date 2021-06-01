#include "TcpServer.hpp"
#include "Logger.hpp"
#include <stdexcept>

TcpServer::TcpServer()
{
    _service = g_socket_service_new();
    GError *error{};
    _port = g_socket_listener_add_any_inet_port(G_SOCKET_LISTENER(_service), nullptr, &error);

    if (!_port && error)
    {
        throw std::runtime_error(error->message);
        //g_clear_error (&error);
    }
    Logger().info("Listening to TCP port {}", _port);

    using IncomingCbT = gboolean (*)(GSocketService *service,
                                     GSocketConnection *,
                                     GObject *source_object,
                                     gpointer user_data);
    IncomingCbT incoming_cb = [](GSocketService *,
                                 GSocketConnection *connection,
                                 GObject * /*source_object*/,
                                 gpointer data) -> gboolean {
        Logger().info("Received incoming connection");
        TcpServer *self = reinterpret_cast<TcpServer *>(data);
        self->_OnConnection(connection);
        return false;
    };

    g_signal_connect(_service, "incoming", G_CALLBACK(incoming_cb), this);
    g_socket_service_start(_service);
}

TcpServer::~TcpServer()
{
    g_socket_service_stop(_service);
}

void TcpServer::_OnConnection(GSocketConnection *connection)
{
    ConnData *data = new ConnData(this, connection);
    _ReceiveMessage(data);
}

void TcpServer::_ReceiveMessage(ConnData *data)
{
    GInputStream *istream = g_io_stream_get_input_stream(G_IO_STREAM(data->connection));

    using MessageReadyT = void (*)(GObject * /*source_object*/,
            GAsyncResult *,
            gpointer);
    MessageReadyT message_ready = [](GObject *, GAsyncResult *res, gpointer data) {
        ConnData *conn_data = reinterpret_cast<ConnData *>(data);
        conn_data->tcp_server->_MessageReady(res, conn_data);
    };

    g_input_stream_read_async(istream,
                              data->message,
                              sizeof(data->message),
                              G_PRIORITY_DEFAULT,
                              nullptr,
                              message_ready,
                              data);
}

void TcpServer::_MessageReady(GAsyncResult *res, ConnData *data)
{
    GInputStream *istream = g_io_stream_get_input_stream(G_IO_STREAM(data->connection));

    GError *error = NULL;
    int count = g_input_stream_read_finish(istream, res, &error);
    if (count == -1)
    {
        Logger().warn("Error receiving from TCP: {}", error ? error->message : "?");
        //if (error)
        //    g_clear_error (&error);
    }
    if (!count)
    {
        Logger().info("Connection closed");
        delete data;
        return;
    }
    Logger().info("Message: {}", std::string_view{data->message, size_t(count)});
    _ReceiveMessage(data);
}
