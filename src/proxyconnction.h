#ifndef CONNHANDLER_H
#define CONNHANDLER_H

#include <boost/unordered_map.hpp>

#include "httpparser.h"
#include "requesthandler.h"
#include "common.h"

namespace msystem {

class ConfigPool;
class ConnManager;

class Connection : public std::enable_shared_from_this<Connection>  {
 public:
  Connection(ba::io_context& io_ctx);
  void Run();
  void HandleServerProxyWrite(const bs::error_code& ec, size_t len);
  void HandleServerProxyRead(const bs::error_code& ec, size_t len);
  void HandleClientProxyWrite(const bs::error_code& ec, size_t len);
  void HandleClientProxyRead(const bs::error_code& ec, size_t len);
  void CheckDeadline();
  void ConnectToServer();
  void HandleResolve(const bs::error_code& ec, ba::ip::tcp::resolver::iterator endpoint_iterator);
  void HandleConnect(const bs::error_code& ec, ba::ip::tcp::resolver::iterator endpoint_iterator);
  void WriteHTTPRequestToServer();
  void WriteRawRequestToServer();
  void HandleServerWrite(const bs::error_code& ec, size_t len);
  void HandleServerRead(const bs::error_code& ec, size_t len);
  void HandleClientWrite(const bs::error_code& ec, size_t len);
  void HandleSslClientWrite(const bs::error_code& ec, size_t len);
  HttpProtocol& GetHttpProtocol() {
    return http_;
  }
  ba::ip::tcp::socket& Socket() {
      return client_socket_;
  }
  void Reset() {
    cli_recv_buffer_.fill('\0');
    ser_recv_buffer_.fill('\0');
    http_.Reset();
    server_response_.Reset();
    http_parser_.Reset(&http_);
    is_persistent_ = false;
    is_proxy_connected_ = false;
  }
  void Shutdown();
  ~Connection();
 private:
  void ReadRequest(const bs::error_code& ec, size_t len);
  void ComposeRequestByProtocol(const HttpProtocol& http, std::string& http_str);
  void ComposeResponseByProtocol(const HttpProtocol& http, std::string& http_str);
  void GetSslResponse(std::string& http_str);
  void EstablishHttpConnection(HttpProtocol& http, std::string& http_str);
  int ProcessRequest();
  int ExtractUrl(const std::string& url, int default_port);
  void StripUserNameAndPassword(std::string& host);
  int StripReturnPort(std::string& host);
  void RemoveConnectionHeaders();
  ba::io_context& io_ctx_;
  ConfigPool* config_pool_;
  std::array<char, 8129> cli_recv_buffer_;
  std::array<char, 8129> ser_recv_buffer_;

  HttpProtocol http_;
  HttpProtocol server_response_;
  HttpParser http_parser_;
  RequestHandler request_handler_;
  ba::ip::tcp::socket client_socket_;
  ba::ip::tcp::socket server_socket_;
  ba::ip::tcp::resolver resolver_;
  ba::steady_timer deadline_;
  uint8_t is_upstream_;
  uint8_t is_server_opened_;
  uint8_t is_proxy_connected_;
  uint8_t is_persistent_;
};
}

#endif
