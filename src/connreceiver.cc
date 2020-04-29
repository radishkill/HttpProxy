#include "connreceiver.h"

#include <iostream>

#include <boost/bind.hpp>

#include "conf.h"
#include "connhandler.h"
#include "filter.h"

namespace msystem {

ConnReceiver::ConnReceiver(io_context& io_ctx)
    : io_ctx_(io_ctx),
      acceptor_(io_ctx),
      config_pool_(msystem::ConfigPool::GetConfigPool()),
      conn_manager() {

  if (config_pool_->listen_addrs.empty())
    throw "bind_address in config file is necessary";

  std::string address = config_pool_->listen_addrs[0];
  std::string port = std::to_string(config_pool_->port);

  asio::ip::tcp::resolver resolver(io_ctx);
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
}



int ConnReceiver::StartAccept() {
  conn_handler_ptr_ = std::make_shared<ConnHandler>(conn_manager);
  acceptor_.async_accept(conn_handler_ptr_->GetConnInfo().client_socket,
                         boost::bind(&ConnReceiver::HandleClient, this, boost::asio::placeholders::error));
  return 0;
}



ConnReceiver::~ConnReceiver() {

}

void ConnReceiver::HandleClient(const boost::system::error_code &error) {
  if (!acceptor_.is_open())
    return;
  if (!error) {
    conn_manager.Start(conn_handler_ptr_);
  } else {
    std::cout << error.message() << std::endl;
  }
  StartAccept();
}


}
