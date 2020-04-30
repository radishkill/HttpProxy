#include "proxyconnction.h"

#include <iostream>
#include <string>

#include <boost/asio.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/bind.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/lambda/lambda.hpp>

#include "conf.h"
#include "connmanager.h"
#include "response.h"
#include "filter.h"
#include "utils.h"

namespace msystem {
namespace asio = boost::asio;

Connection::Connection(ba::io_context& io_ctx)
  : io_ctx_(io_ctx),
    http_parser(&http_),
    request_handler_(*this),
    client_socket_(io_ctx_),
    server_socket_(io_ctx_),
    resolver_(io_ctx_),
    deadline_(io_ctx_),
    is_opened(false) {
  this->config_pool_ = ConfigPool::GetConfigPool();
}

void Connection::Run() {
//  auto remote_endpoint = conn_info_.client_socket.remote_endpoint();
//  std::cout << "get connect from\n";
//  std::cout << remote_endpoint.address().to_string() << ":"
//            << remote_endpoint.port() << "\n";
//  std::cout << "thread_id:" << std::this_thread::get_id() << std::endl;
  http_parser.Reset(&http_);
  ReadRequest(bs::error_code(), 0);
}

void Connection::WriteToServer() {
  auto self = shared_from_this();
//  server_socket_.async_write_some(ser_send_buffer_,
//      [this, self](const boost::system::error_code& ec, std::size_t bytes_transferred) {
//    if (ec) {
////      if (stopped_)
////        return ;
//      Shutdown();
//    }
//    if (state_ == kConnToServer)
//      state_ = kProxySuccess;
//  });
}

void Connection::ReadFromServer() {
  auto self = shared_from_this();
  ba::async_read(server_socket_, asio::buffer(ser_recv_buffer_),
        [this, self](const boost::system::error_code& ec, std::size_t len) {
    if (ec) {
      Shutdown();
      return;
    }
    std::cout << std::string(ser_recv_buffer_.data(), len) << std::endl;
  });
}

void Connection::WriteToClient() {
  auto self = shared_from_this();
//  client_socket_.async_write_some(cli_send_buffer_,
//                                            [this, self](const boost::system::error_code& ec, std::size_t) {
//    if (ec) {
////      if (stopped_)
////        return ;
//      return;
//    }
//    if (state_ == kConnToServer) {
//      state_ = kProxySuccess;
//    }
//  });
}

void Connection::ReadFromClient() {
  auto self = shared_from_this();
//  client_socket_.async_receive(asio::buffer(cli_recv_buffer_),
//                                         [this, self](const boost::system::error_code& ec, std::size_t bytes_transferred) {
//    if (ec) {
////      if (stopped_)
////        return ;
//      return ;
//    }
//    ser_send_buffer_.clear();
//    ser_send_buffer_.push_back(boost::asio::buffer(cli_recv_buffer_, bytes_transferred));
//    WriteToServer();
//    ReadFromClient();

//  });
}

void Connection::CheckDeadline() {
//  if (stopped_)
//    return;
  auto self = shared_from_this();
  //超时
  if (deadline_.expiry() <= asio::steady_timer::clock_type::now()) {
    if (state_ == kRequestFromClient) {
      std::cout << "connect time out" << std::endl;
//      Stop(self);
      return;
    }
    deadline_.expires_at(asio::steady_timer::time_point::max());
  }
  deadline_.async_wait(std::bind(&Connection::CheckDeadline, this));
}
void Connection::ConnectToServer() {
  if (!is_opened) {
    ba::ip::tcp::resolver::query query(http_.host, std::to_string(http_.port));
    resolver_.async_resolve(query,
                    boost::bind(&Connection::HandleResolve, shared_from_this(),
                          boost::asio::placeholders::error,
                          boost::asio::placeholders::iterator));
  } else {
    WriteRequestToServer();
  }

//  auto self = shared_from_this();
//  boost::system::error_code ec;
//  asio::ip::tcp::resolver resolver(io_ctx_);
//  asio::ip::tcp::endpoint endpoint;
//  try {
//     endpoint = *resolver.resolve(http_.host, std::to_string(http_.port), ec).begin();
//  }  catch (...) {
//    return -1;
//  }
//  //deadline_.expires_after(std::chrono::seconds(config_pool_->idletimeout));

//  server_socket.async_connect(endpoint, [&](const boost::system::error_code& ec) {
//    if (ec) {
////      if (stopped_ && !client_socket.is_open())
////        return ;
////      std::cout << ec.value() << ":" << ec.message() << std::endl;
////      std::string str_response;
////      Response::ExpectationFailed(str_response);
////      cli_send_buffer_.push_back(asio::buffer(str_response));
////      client_socket.write_some(asio::buffer(cli_send_buffer_));
////      conn_manager_.Stop(self);
//      return ;
//    }
//  });
//  io_ctx_.restart();
//  io_ctx_.run_for(std::chrono::seconds(config_pool_->idletimeout));
//  if (!server_socket.is_open())
//    return -1;
//  state_ = kConnToServer;
//  if (http_.connect_method) {
//    std::string str_response;
//    Response::GetSslResponse(str_response);
//    cli_send_buffer_.clear();
//    cli_send_buffer_.push_back(boost::asio::buffer(str_response));
//    WriteToClient();
//    Response::EstablishHttpConnection(http_, str_response);
//    WriteToServer();
//    ReadFromClient();
//    ReadFromServer();
//  } else {
//    std::string str_req;
//    RequestHandler::ComposeRequest(http_, str_req);
//    ser_send_buffer_.clear();
//    ser_send_buffer_.push_back(boost::asio::buffer(str_req));
//    WriteToServer();
//    ReadFromServer();
//    ReadFromClient();
//  }
}
void Connection::HandleResolve(const boost::system::error_code &ec, ba::ip::tcp::resolver::iterator endpoint_iterator) {
  if (ec) {
    Shutdown();
    return;
  }
  const bool first_time = true;
  HandleConnect(boost::system::error_code(), endpoint_iterator, first_time);
}

void Connection::HandleConnect(const boost::system::error_code &ec, ba::ip::tcp::resolver::iterator endpoint_iterator, const bool first_time) {
  if (ec) {
    std::cout << ec.message() << std::endl;
    Shutdown();
    return;
  }
  if (!first_time) {
    is_opened = true;
//    auto remote_endpoint = server_socket_.remote_endpoint();
//    std::cout << "get connect from\n";
//    std::cout << remote_endpoint.address().to_string() << ":"
//              << remote_endpoint.port() << "\n";
    if (http_.connect_method) {
      std::cout << "connect out\n";
      return;
    }
    WriteRequestToServer();
  } else if (endpoint_iterator != ba::ip::tcp::resolver::iterator()) {
    ba::ip::tcp::endpoint endpoint = *endpoint_iterator;
    server_socket_.async_connect(endpoint,
        boost::bind(&Connection::HandleConnect, shared_from_this(),
            boost::asio::placeholders::error,
            ++endpoint_iterator, false));
  } else {
    Shutdown();
  }
}

void Connection::WriteRequestToServer() {
  std::string req_buffer;
  req_buffer += http_.method;
  req_buffer += " ";
  req_buffer += http_.raw_url;
  req_buffer += " ";
  req_buffer += http_.http_version;
  req_buffer += "\r\n";
  for (const auto &h : http_.headers) {
    req_buffer += h.first;
    req_buffer += ": ";
    req_buffer += h.second;
    req_buffer += "\r\n";
  }
  req_buffer += "\r\n";
  ba::async_write(server_socket_, ba::buffer(req_buffer),
      boost::bind(&Connection::HandleServerWrite,
          shared_from_this(),
          ba::placeholders::error,
          ba::placeholders::bytes_transferred));
}

void Connection::HandleServerWrite(const boost::system::error_code &ec, size_t len) {
  if (ec) {
    Shutdown();
    return;
  }
  ser_data.clear();
  http_parser.Reset(&server_response);
  HandleServerRead(bs::error_code(), 0);
}

void Connection::HandleServerRead(const boost::system::error_code &ec, size_t len) {
  if (ec) {
    Shutdown();
    return;
  }
  HttpParser::ResultType result;
  std::tie(result, std::ignore) = http_parser.ParseResponse(ser_recv_buffer_.data(), ser_recv_buffer_.data() + len);
  if (result == HttpParser::kGood) {
    std::cout << "url: " << http_.url << "\n";
    std::cout << "method: " << http_.method << "\n";
    std::cout << "version: " << http_.http_version << "\n";
    for (const auto& h : http_.headers) {
      std::cout << h.first << ":" << h.second << "\n";
    }
    std::cout << http_.data << std::endl;
    std::cout << http_.data_length << std::endl;

//    std::string response_version = ser_data.substr(ser_data.find("HTTP/")+5, 3);
//    ba::async_write(client_socket_, ba::buffer(ser_data),
//        boost::bind(&Connection::HandleClientWrite,
//            shared_from_this(),
//            ba::placeholders::error,
//            ba::placeholders::bytes_transferred));
  } else if (result == HttpParser::kBad) {
    Shutdown();
    return;
  } else {
    ba::async_read(server_socket_, ba::buffer(ser_recv_buffer_), ba::transfer_at_least(1),
        boost::bind(&Connection::HandleServerRead,
            shared_from_this(),
            ba::placeholders::error,
            ba::placeholders::bytes_transferred));
  }
}

void Connection::HandleClientWrite(const boost::system::error_code& ec, size_t len) {
  if (ec) {
    Shutdown();
    return;
  }
  std::cout << "wirte success" << len << "\n";
}


void Connection::Shutdown() {
  boost::system::error_code ignored_ec;
  if (server_socket_.is_open()) {
    server_socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
    server_socket_.close();
  }

  if (client_socket_.is_open()) {
    server_socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
    server_socket_.close();
  }
}

Connection::~Connection() {

}

void Connection::ReadRequest(const bs::error_code& ec, size_t len) {
  if (ec) {
    std::cout << ec.message() << std::endl;
    Shutdown();
    return;
  }
  HttpParser::ResultType result;
  std::tie(result, std::ignore) = http_parser.ParseRequest(cli_recv_buffer_.data(), cli_recv_buffer_.data() + len);
  if (result == HttpParser::kGood) {
    std::cout << "url: " << http_.raw_url << "\n";
    std::cout << "method: " << http_.method << "\n";
    std::cout << "version: " << http_.http_version << "\n";
    for (const auto& h : http_.headers) {
      std::cout << h.first << ":" << h.second << "\n";
    }
    if (request_handler_.ProcessRequest() < 0) {
      Shutdown();
      return;
    }
    ConnectToServer();
  } else if (result == HttpParser::kBad) {
    Shutdown();
    return;
  } else {
    ba::async_read(client_socket_, ba::buffer(cli_recv_buffer_), ba::transfer_at_least(1),
        boost::bind(&Connection::ReadRequest,
            shared_from_this(),
            ba::placeholders::error,
            ba::placeholders::bytes_transferred));
  }
}



}
