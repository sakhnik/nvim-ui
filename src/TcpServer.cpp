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
    IncomingCbT incoming_cb = [](GSocketService *service,
                                 GSocketConnection *connection,
                                 GObject *source_object,
                                 gpointer user_data) -> gboolean {
        Logger().info("Received incoming connection");
        //GInputStream *istream = g_io_stream_get_input_stream(G_IO_STREAM(connection));
        //struct ConnData *data = g_new (struct ConnData, 1);

        //data->connection = g_object_ref (connection);

        //g_input_stream_read_async (istream,
        //    data->message,
        //    sizeof (data->message),
        //    G_PRIORITY_DEFAULT,
        //    NULL,
        //    message_ready,
        //    data);
        return false;
    };

    g_signal_connect(_service, "incoming", G_CALLBACK(incoming_cb), this);
    g_socket_service_start(_service);
}

TcpServer::~TcpServer()
{
    g_socket_service_stop(_service);
}

//#include <gio/gio.h>
//#include <glib.h>
//
//#define BLOCK_SIZE 1024
//#define PORT 2345
//
//struct ConnData {
//  GSocketConnection *connection;
//  char message[BLOCK_SIZE];
//};
//
//void message_ready (GObject * source_object,
//    GAsyncResult *res,
//    gpointer user_data)
//{
//  GInputStream *istream = G_INPUT_STREAM (source_object);
//  GError *error = NULL;
//  struct ConnData *data = user_data;
//  int count;
//
//  count = g_input_stream_read_finish (istream,
//      res,
//      &error);
//
//  if (count == -1) {
//    g_error ("Error when receiving message");
//    if (error != NULL) {
//      g_error ("%s", error->message);
//      g_clear_error (&error);
//    }
//  }
//  g_message ("Message was: \"%s\"\n", data->message);
//  g_object_unref (G_SOCKET_CONNECTION (data->connection));
//  g_free (data);
//}
