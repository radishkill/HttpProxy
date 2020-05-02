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
    http_parser_(&http_),
    request_handler_(*this),
    client_socket_(io_ctx_),
    server_socket_(io_ctx_),
    resolver_(io_ctx_),
    deadline_(io_ctx_),
    is_server_opened_(false),
    is_proxy_connected_(false),
    is_persistent_(false) {
  this->config_pool_ = ConfigPool::GetConfigPool();
}

void Connection::Run() {
//  auto remote_endpoint = conn_info_.client_socket.remote_endpoint();
//  std::cout << "get connect from\n";
//  std::cout << remote_endpoint.address().to_string() << ":"
//            << remote_endpoint.port() << "\n";
//  std::cout << "thread_id:" << std::this_thread::get_id() << std::endl;
  Reset();
  ReadRequest(bs::error_code(), 0);
}

void Connection::HandleServerProxyWrite(const bs::error_code& ec, size_t len) {
  if (ec) {
    Shutdown();
    return;
  }
  ba::async_read(client_socket_, ba::buffer(cli_recv_buffer_), ba::transfer_at_least(1),
      boost::bind(&Connection::HandleClientProxyRead,
          shared_from_this(),
          ba::placeholders::error,
          ba::placeholders::bytes_transferred));
}

void Connection::HandleServerProxyRead(const bs::error_code& ec, size_t len) {
  if (ec) {
    Shutdown();
    return;
  }
  ba::async_write(client_socket_, ba::buffer(ser_recv_buffer_, len),
      boost::bind(&Connection::HandleClientProxyWrite,
          shared_from_this(),
          ba::placeholders::error,
          ba::placeholders::bytes_transferred));
}

void Connection::HandleClientProxyWrite(const bs::error_code& ec, size_t len) {
  if (ec) {
    Shutdown();
    return;
  }
  ba::async_read(server_socket_, ba::buffer(ser_recv_buffer_), ba::transfer_at_least(1),
      boost::bind(&Connection::HandleServerProxyRead,
          shared_from_this(),
          ba::placeholders::error,
          ba::placeholders::bytes_transferred));
}

void Connection::HandleClientProxyRead(const bs::error_code& ec, size_t len) {
  if (ec) {
    Shutdown();
    return;
  }
  ba::async_write(server_socket_, ba::buffer(cli_recv_buffer_, len),
      boost::bind(&Connection::HandleServerProxyWrite,
          shared_from_this(),
          ba::placeholders::error,
          ba::placeholders::bytes_transferred));
}

void Connection::CheckDeadline() {
//  if (stopped_)
//    return;
  auto self = shared_from_this();
  //超时
  if (deadline_.expiry() <= asio::steady_timer::clock_type::now()) {
//    if (state_ == kRequestFromClient) {
//      std::cout << "connect time out" << std::endl;
////      Stop(self);
//      return;
//    }
    deadline_.expires_at(asio::steady_timer::time_point::max());
  }
  deadline_.async_wait(std::bind(&Connection::CheckDeadline, this));
}
void Connection::ConnectToServer() {
  if (!is_server_opened_) {
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
    is_server_opened_ = true;
    //HTTPS connection
    if (http_.connect_method) {
      std::string response_str;
      GetSslResponse(response_str);
      ba::async_write(client_socket_, ba::buffer(response_str),
          boost::bind(&Connection::HandleSslClientWrite,
              shared_from_this(),
              ba::placeholders::error,
              ba::placeholders::bytes_transferred));
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
  http_parser_.Reset(&server_response_);
  HandleServerRead(bs::error_code(), 0);
}

void Connection::HandleServerRead(const boost::system::error_code &ec, size_t len) {
  if (ec) {
    Shutdown();
    return;
  }
  HttpParser::ResultType result;
  std::tie(result, std::ignore) = http_parser_.ParseResponse(ser_recv_buffer_.data(), ser_recv_buffer_.data() + len);
  if (result == HttpParser::kGood) {
    std::string response_str;
    std::string response_connection_str;
    std::string request_connection_str;
    auto iter = server_response_.headers.find("connection");
    if(iter != server_response_.headers.end())
      response_connection_str = iter->second;
    iter = http_.headers.find("connection");
    if(iter != http_.headers.end())
      request_connection_str = iter->second;

    is_persistent_ = (
          ((http_.http_version == "HTTP/1.1" && request_connection_str != "close") ||
           (http_.http_version == "HTTP/1.0" && request_connection_str == "keep-alive")) &&
           ((server_response_.http_version == "HTTP/1.1" && response_connection_str != "close") ||
           (server_response_.http_version == "HTTP/1.0" && response_connection_str == "keep-alive"))
          );

    ComposeResponseByProtocol(server_response_, response_str);
    ba::async_write(client_socket_, ba::buffer(response_str),
        boost::bind(&Connection::HandleClientWrite,
            shared_from_this(),
            ba::placeholders::error,
            ba::placeholders::bytes_transferred));
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
  is_proxy_connected_ = true;
  if (is_persistent_) {
    Run();
  }
}

void Connection::HandleSslClientWrite(const boost::system::error_code& ec, size_t len) {
  if (ec) {
    Shutdown();
    return;
  }
  ba::async_read(client_socket_, ba::buffer(cli_recv_buffer_), ba::transfer_at_least(1),
      boost::bind(&Connection::HandleClientProxyRead,
          shared_from_this(),
          ba::placeholders::error,
          ba::placeholders::bytes_transferred));
  ba::async_read(server_socket_, ba::buffer(ser_recv_buffer_), ba::transfer_at_least(1),
      boost::bind(&Connection::HandleServerProxyRead,
          shared_from_this(),
          ba::placeholders::error,
            ba::placeholders::bytes_transferred));
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
  std::tie(result, std::ignore) = http_parser_.ParseRequest(cli_recv_buffer_.data(), cli_recv_buffer_.data() + len);
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

void Connection::ComposeRequestByProtocol(const HttpProtocol &http, std::string &http_str) {
  http_str.clear();
  http_str += http.method;
  http_str.push_back(' ');
  http_str += http.raw_url;
  http_str.push_back(' ');
  http_str += http.http_version;
  http_str += "\r\n";
  for (const auto &h : http.headers) {
    http_str += h.first;
    http_str += ": ";
    http_str += h.second;
    http_str += "\r\n";
  }
  http_str += "\r\n";
  http_str += http.data;
}

void Connection::ComposeResponseByProtocol(const HttpProtocol &http, std::string &http_str) {
  http_str.clear();
  http_str += http.http_version;
  http_str.push_back(' ');
  http_str += http.status_code;
  http_str.push_back(' ');
  http_str += http.reason_phrase;
  http_str += "\r\n";
  for (const auto &h : http.headers) {
    http_str += h.first;
    http_str += ": ";
    http_str += h.second;
    http_str += "\r\n";
  }
  http_str += "\r\n";
  http_str += http.data;
}

void Connection::GetSslResponse(std::string& http_str) {
  http_str.clear();
  http_str += "HTTP/1.0 200 Connection established\r\n";
  http_str += "Proxy-agent: httpproxy/1.0\r\n\r\n";
}

void Connection::EstablishHttpConnection(HttpProtocol& http, std::string& http_str) {
  http_str = http.method;
  http_str += ' ';
  http_str += http.raw_url;
  http_str += " HTTP/1.0\r\n";
  http_str += "Host: ";
  http_str += http.host;
  http_str += ':';
  http_str += std::to_string(http.port);
  http_str += "\r\n";
  http_str += "Connection: close\r\n\r\n";
}
}

