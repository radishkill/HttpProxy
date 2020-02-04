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
  ConnReceiver(io_context& io_context_acceptor, io_context& io_context_handler);
  int StartAccept();
  ~ConnReceiver();
 private:
  io_context& io_context_acceptor_;
  io_context& io_context_handler_;
  tcp::acceptor acceptor_;
  std::vector<tcp::acceptor*> acceptors_;
  std::vector<tcp::socket*> sockets_;
  ConfigPool* config_pool_;
  ConnManager conn_manager;
};
}

#endif // CONNRECEIVER_H
