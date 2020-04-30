#ifndef PROXY_SERVER_H
#define PROXY_SERVER_H

#include <vector>
#include <thread>

#include <boost/asio.hpp>

#include "common.h"
#include "proxyconnction.h"

using boost::asio::io_context;
using boost::asio::ip::tcp;
using boost::asio::ip::address_v4;
namespace asio = boost::asio;

namespace msystem {
typedef std::deque<io_context_ptr> ioctx_deque;

class ConfigPool;

class ProxyServer {
 public:
  ProxyServer(const ioctx_deque& io_contexts);
  int StartAccept();
  ~ProxyServer();
 private:
  void HandleClient(std::shared_ptr<Connection> new_connection, const boost::system::error_code &error);
  ioctx_deque io_contexts_;
  tcp::acceptor acceptor_;
  ConfigPool* config_pool_;

};
}

#endif // CONNRECEIVER_H
