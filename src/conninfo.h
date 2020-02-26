#ifndef CONNINFO_H
#define CONNINFO_H

#include <boost/asio.hpp>

#include "utils/fifo.h"

namespace msystem {

namespace asio = boost::asio;
/*
 * To store connection data;
 */
class ConnInfo {
 public:
  ConnInfo(const ConnInfo&) = delete;
  ConnInfo& operator=(const ConnInfo&) = delete;

  explicit ConnInfo(asio::ip::tcp::socket c_socket);

  asio::ip::tcp::socket client_socket;
  asio::ip::tcp::socket server_socket;

  uint8_t connect_method;
  enum State {
    kConnInitial,
    kRequestFromClient,
    kConnToServer,
    kProxySuccess
  } state_;
 private:

};

}
#endif
