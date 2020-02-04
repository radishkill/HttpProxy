#include "connreceiver.h"

#include <iostream>

#include <boost/bind.hpp>

#include "conf.h"
#include "connhandler.h"

namespace msystem {

ConnReceiver::ConnReceiver(io_context& io_context_acceptor, io_context& io_context_handler)
    : io_context_acceptor_(io_context_acceptor),
      io_context_handler_(io_context_handler),
      acceptor_(io_context_acceptor),
      config_pool_(msystem::ConfigPool::GetConfigPool()),
      conn_manager() {

  if (config_pool_->listen_addrs.empty())
    throw "bind_address in config file is necessary";
  std::string address = config_pool_->listen_addrs[0];
  std::string port = std::to_string(config_pool_->port);

  asio::ip::tcp::resolver resolver(io_context_acceptor);
  asio::ip::tcp::endpoint listen_endpoint = *resolver.resolve(address, port).begin();

  this->acceptor_.open(listen_endpoint.protocol());
  this->acceptor_.set_option(asio::ip::tcp::socket::reuse_address());
  this->acceptor_.bind(listen_endpoint);
  this->acceptor_.listen();

  std::vector<std::string>::iterator iter = config_pool_->listen_addrs.begin();
  for (; iter != config_pool_->listen_addrs.end(); ++iter) {
    if (iter == config_pool_->listen_addrs.begin())
      continue;
    boost::asio::ip::address addr = boost::asio::ip::make_address((*iter).c_str());
    this->acceptor_.set_option(boost::asio::ip::multicast::join_group(addr));
  }
  StartAccept();
}

int ConnReceiver::StartAccept() {
  acceptor_.async_accept(io_context_handler_ ,[this](const boost::system::error_code &ec, tcp::socket socket) {
    std::cout << "[tid:" << std::this_thread::get_id() << "] \n";
    if (!ec) {
      conn_manager.Start(std::make_shared<ConnHandler>(std::move(socket), conn_manager));
    }
    StartAccept();
  });
  return 0;
}



ConnReceiver::~ConnReceiver() {

}


}
