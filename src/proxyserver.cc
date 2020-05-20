#include "proxyserver.h"

#include <iostream>

#include <boost/bind.hpp>

#include "conf.h"
#include "proxyconnction.h"
#include "filter.h"
#include "common.h"

namespace msystem {

ProxyServer::ProxyServer(const ioctx_deque& io_contexts)
    : io_contexts_(io_contexts),
      acceptor_(*io_contexts_.front()),
      config_pool_(ConfigPool::GetConfigPool()) {

  if (config_pool_->listen_addrs.empty())
    throw "bind_address in config file is necessary";

  std::string address = config_pool_->listen_addrs[0];
  std::string port = std::to_string(config_pool_->port);

  asio::ip::tcp::resolver resolver(*io_contexts_.front());
  asio::ip::tcp::endpoint listen_endpoint = *resolver.resolve(address, port).begin();
  acceptor_.open(listen_endpoint.protocol());
  acceptor_.set_option(asio::ip::tcp::socket::reuse_address());
  acceptor_.bind(listen_endpoint);
  acceptor_.listen();

  std::vector<std::string>::iterator iter = config_pool_->listen_addrs.begin();
  for (; iter != config_pool_->listen_addrs.end(); ++iter) {
    if (iter == config_pool_->listen_addrs.begin())
      continue;
    boost::asio::ip::address addr = boost::asio::ip::make_address((*iter).c_str());
    acceptor_.set_option(boost::asio::ip::multicast::join_group(addr));
  }
}



int ProxyServer::StartAccept() {
  io_contexts_.push_back(io_contexts_.front());
  io_contexts_.pop_front();
  std::shared_ptr<Connection> new_connection = std::make_shared<Connection>(*io_contexts_.front());
  acceptor_.async_accept(new_connection->Socket(),
                         boost::bind(&ProxyServer::HandleClient, this, new_connection, ba::placeholders::error));
  return 0;
}

void ProxyServer::HandleClient(std::shared_ptr<Connection> new_connection, const boost::system::error_code &error) {
  if (error) {
    std::cout << error.message() << std::endl;
    return;
  }
  new_connection->Run();
  StartAccept();
}
ProxyServer::~ProxyServer() {

}

}
