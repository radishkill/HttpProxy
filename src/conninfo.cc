#include "conninfo.h"


namespace msystem {

ConnInfo::ConnInfo(asio::ip::tcp::socket c_socket)
    : client_socket(std::move(c_socket)),
      server_socket(client_socket.get_executor()),
      connect_method(false),
      state_(kConnInitial) {
}

}
