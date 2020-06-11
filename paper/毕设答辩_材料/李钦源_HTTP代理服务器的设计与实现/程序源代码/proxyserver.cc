#include "proxyserver.h"

#include <iostream>

#include <boost/bind.hpp>

#include "conf.h"
#include "proxyconnection.h"
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

  //解析地址
  asio::ip::tcp::resolver resolver(*io_contexts_.front());
  asio::ip::tcp::endpoint listen_endpoint = *resolver.resolve(address, port).begin();
  //初始化监听端口
  acceptor_.open(listen_endpoint.protocol());
  acceptor_.set_option(asio::ip::tcp::socket::reuse_address());
  acceptor_.bind(listen_endpoint);
  acceptor_.listen();
  //对多个网口监听进行处理
  std::vector<std::string>::iterator iter = config_pool_->listen_addrs.begin();
  for (; iter != config_pool_->listen_addrs.end(); ++iter) {
    if (iter == config_pool_->listen_addrs.begin())
      continue;
    boost::asio::ip::address addr = boost::asio::ip::make_address((*iter).c_str());
    acceptor_.set_option(boost::asio::ip::multicast::join_group(addr));
  }
}



int ProxyServer::StartAccept() {
  //从线程池中取出线程用于执行监听
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
  //处理请求并再次开始监听
  new_connection->Run();
  StartAccept();
}
ProxyServer::~ProxyServer() {

}

}
