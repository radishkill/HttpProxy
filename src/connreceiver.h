#ifndef CONNRECEIVER_H
#define CONNRECEIVER_H

#include <vector>
#include <thread>

#include <boost/asio.hpp>

#include "connmanager.h"

using boost::asio::io_context;
using boost::asio::ip::tcp;
using boost::asio::ip::address_v4;
namespace asio = boost::asio;

namespace msystem {

class ConfigPool;

class ConnReceiver {
 public:
  ConnReceiver(io_context& io_ctx);
  int StartAccept();
  ~ConnReceiver();
 private:
  void HandleClient(const boost::system::error_code& error);
  io_context& io_ctx_;
  tcp::acceptor acceptor_;
  std::vector<tcp::acceptor*> acceptors_;
  std::vector<tcp::socket*> sockets_;
  ConfigPool* config_pool_;
  ConnManager conn_manager;
  std::shared_ptr<ConnHandler> conn_handler_ptr_;
};
}

#endif // CONNRECEIVER_H
