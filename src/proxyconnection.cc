#include "proxyconnection.h"

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
    is_upstream_(false),
    is_reverse_(false),
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
//  std::cout << std::endl;
//  std::cout << ser_recv_buffer_.data() << std::endl;
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
    WriteHTTPRequestToServer();
  }
}
void Connection::HandleResolve(const boost::system::error_code &ec, ba::ip::tcp::resolver::iterator endpoint_iterator) {
  if (ec) {
    Shutdown();
    return;
  }
  HandleConnect(boost::system::error_code(), endpoint_iterator);
}

void Connection::HandleConnect(const boost::system::error_code &ec, ba::ip::tcp::resolver::iterator endpoint_iterator) {
  if (ec) {
    std::cout << ec.message() << std::endl;
    Shutdown();
    return;
  }
  if (server_socket_.is_open()) {
    is_server_opened_ = true;
    if (is_upstream_) {
      WriteRawRequestToServer();
      return;
    }
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
    WriteHTTPRequestToServer();
  } else if (endpoint_iterator != ba::ip::tcp::resolver::iterator()) {
    ba::ip::tcp::endpoint endpoint = *endpoint_iterator;
    server_socket_.async_connect(endpoint,
        boost::bind(&Connection::HandleConnect, shared_from_this(),
            boost::asio::placeholders::error,
            ++endpoint_iterator));
  } else {
    Shutdown();
  }
}

void Connection::WriteHTTPRequestToServer() {
  std::string req_buffer;
  EstablishHttpConnection(http_, req_buffer);
  ba::async_write(server_socket_, ba::buffer(req_buffer),
      boost::bind(&Connection::HandleServerWrite,
          shared_from_this(),
          ba::placeholders::error,
                  ba::placeholders::bytes_transferred));
}

void Connection::WriteRawRequestToServer() {
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
  ba::async_read(server_socket_, ba::buffer(ser_recv_buffer_), ba::transfer_at_least(1),
      boost::bind(&Connection::HandleServerProxyRead,
          shared_from_this(),
          ba::placeholders::error,
          ba::placeholders::bytes_transferred));
  ReadRequest(bs::error_code(), 0);
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
  //使用解析函数解析请求报文
  std::tie(result, std::ignore) = http_parser_.ParseRequest(cli_recv_buffer_.data(), cli_recv_buffer_.data() + len);
  if (result == HttpParser::kGood) {
    std::cout << "----------------------------------\n";
    std::cout << "url: " << http_.raw_url << "\n";
    std::cout << "method: " << http_.method << "\n";
    std::cout << "version: " << http_.http_version << "\n";
    for (const auto& h : http_.headers) {
      std::cout << h.first << ":" << h.second << "\n";
    }
    //使用处理函数处理请求报文
    if (ProcessRequest() < 0) {
      Shutdown();
      return;
    }

    //连接上级服务器
    ConnectToServer();

  } else if (result == HttpParser::kBad) {
    Shutdown();
    return;
  } else {
    //异步读取请求报文内容
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
  http_str += "HTTP/1.0 200 Connection established\r\n\r\n";
}

void Connection::EstablishHttpConnection(HttpProtocol& http, std::string& http_str) {
  http_str = http.method;
  http_str += ' ';
  http_str += http.url;
  http_str += " HTTP/1.0\r\n";
  http_str += "Host: ";
  http_str += http.host;
  http_str += ':';
  http_str += std::to_string(http.port);
  http_str += "\r\n";
  http_str += "Connection: Keep-alive\r\n\r\n";
}

int Connection::ProcessRequest() {
  std::size_t p;
  ConfigPool* config_pool = ConfigPool::GetConfigPool();

  //解析协议版本
  if (http_.http_version.find_first_of("HTTP/") == 0) {
    std::string& version = http_.http_version;
    p = version.find_first_of('.');
    if (p == std::string::npos)
      return -1;
    try {
      http_.protocol_major = boost::lexical_cast<uint8_t>(version.substr(5, p-5));
      http_.protocol_minor = boost::lexical_cast<uint8_t>(version.substr(p+1));
    }  catch (...) {
      return -1;
    }
  } else {
    return -1;
  }

  if (http_.raw_url.find("http://") == 0 ||
      http_.raw_url.find("ftp://") == 0) {
    std::size_t skiped_type_p = http_.raw_url.find("//") + 2;
    http_.url = http_.raw_url.substr(skiped_type_p);
    ExtractUrl(http_.url, HTTP_PORT);
    http_.connect_method = false;
  } else if (http_.method == "CONNECT") {
    ExtractUrl(http_.raw_url, HTTP_PORT_SSL);
    //need check allowed connect ports
    http_.connect_method = true;
  } else if (http_.raw_url[0] == '/') {
    //反向代理
    is_reverse_ = 1;
    p = 0;
    http_.raw_url.push_back('/');
    for (std::size_t i=1; i<http_.raw_url.length(); ++i) {
      if (http_.raw_url[i] == '/' && (p != i-1)) {
        http_.url_list.push_back(http_.raw_url.substr(p+1, i-p-1));
        p = i;
      }
    }
    for (const auto& v : config_pool->reversepath_list) {
      if (v.first == http_.url_list[0]) {
        http_.headers.find("host")->second = v.second;
        http_.url = v.second;
        for (auto iter = ++http_.url_list.begin(); iter != http_.url_list.end(); ++iter) {
          http_.url += '/';
          http_.url += *iter;
        }
        ExtractUrl(http_.url, HTTP_PORT);
      }
    }
    //url为空，表示表中为找到对应项
    if (http_.url.empty()) {
      return -1;
    }
  }

  //过滤
  Filter* filter = Filter::GetFilter();
  if (filter) {
    if (!config_pool->filter_url&&!filter->FilterByDomain(http_.host)) {
      return -1;
    } else if (config_pool->filter_url&&!filter->FilterByUrl(http_.url)) {
      return -1;
    }
  }

  if (config_pool->bindsame) {
    auto client_endpoint = client_socket_.remote_endpoint();
    http_.host = client_endpoint.address().to_string();
  } else if (!config_pool->upstream.empty()) {
    is_upstream_ = true;
    http_.host = config_pool->upstream;
    http_.port = StripReturnPort(http_.host);
  }
  return 0;
}
int Connection::ExtractUrl(const std::string& url, int default_port) {
  std::size_t p;

  p = url.find_first_of('/');
  if (p == std::string::npos) {
    http_.host = url;
    http_.url = "/";
  } else {
    http_.host = http_.url.substr(0, p);
    http_.url = http_.url.substr(p);
  }
  StripUserNameAndPassword(http_.host);

  http_.port = StripReturnPort(http_.host);
  http_.port = (http_.port != 0) ? http_.port : default_port;

  //remove ipv6 surrounding '[' and ']'
  p = http_.host.find_first_of(']');
  if (p != std::string::npos && http_.host[0] == '[')
    http_.host = http_.host.substr(1, http_.host.length() - 2);

  return 0;
}

void Connection::StripUserNameAndPassword(std::string& host) {
  std::size_t p;
  p = host.find_first_of('@');
  if (p == std::string::npos)
    return;
  host = host.substr(0, p);
}

int Connection::StripReturnPort(std::string& host) {
  std::size_t p;
  int port;
  /* 检查ipv6 */
  p = host.find_first_of(']');
  if (p != std::string::npos)
    return 0;

  p = host.find_first_of(':');
  if (p == std::string::npos)
    return 0;
  ++p;
  try {
    port = std::atoi(host.substr(p).c_str());
  }  catch (...) {
    return 0;
  }
  --p;
  host = host.substr(0, p);
  return port;
}

void Connection::RemoveConnectionHeaders() {
  static const char* const headers[] = {
    "connection",
    "proxy-connection"
  };
  int i;
  const char* ptr;
  for (i = 0; i != sizeof (headers) / sizeof (char *); i++) {
    auto iter = http_.headers.find(headers[i]);
    if (iter == http_.headers.end())
      continue;
    int len = iter->second.length();
    ptr = iter->second.c_str();
    while ((ptr = std::strpbrk(iter->second.c_str(), "()<>@,;:\\\"/[]?={} \t")))
      iter->second[ptr - iter->second.c_str()] = '\0';
    ptr = iter->second.c_str();
    while (ptr < iter->second.c_str() + len) {

      auto iter1 = http_.headers.find(std::string(ptr));
      if (iter1 != http_.headers.end())
        http_.headers.erase(iter1);
      ptr += std::strlen(ptr)+1;
      while (ptr < iter->second.c_str() + len && *ptr == '\0')
        ptr++;
    }
    http_.headers.erase(iter);
  }
}
}

