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
  void WriteToServer();
  void ReadFromServer();
  void WriteToClient();
  void ReadFromClient();
  void CheckDeadline();
  void ConnectToServer();
  void HandleResolve(const bs::error_code& ec, ba::ip::tcp::resolver::iterator endpoint_iterator);
  void HandleConnect(const bs::error_code& ec, ba::ip::tcp::resolver::iterator endpoint_iterator, const bool first_time);
  void WriteRequestToServer();
  void HandleServerWrite(const bs::error_code& ec, size_t len);
  void HandleServerRead(const bs::error_code& ec, size_t len);
  void HandleClientWrite(const bs::error_code& ec, size_t len);
  HttpProtocol& GetHttpProtocol() {
    return http_;
  }
  ba::ip::tcp::socket& Socket() {
      return client_socket_;
  }
  void Shutdown();
  ~Connection();
 private:
  void ReadRequest(const bs::error_code& ec, size_t len);
  ba::io_context& io_ctx_;
  ConfigPool* config_pool_;
  std::array<char, 8129> cli_recv_buffer_;
  std::array<char, 8129> ser_recv_buffer_;
  std::string ser_data;

  HttpProtocol http_;
  HttpProtocol server_response;
  HttpParser http_parser;
  RequestHandler request_handler_;
  ba::ip::tcp::socket client_socket_;
  ba::ip::tcp::socket server_socket_;
  ba::ip::tcp::resolver resolver_;

  enum State {
    kConnInitial,
    kRequestFromClient,
    kConnToServer,
    kProxySuccess
  } state_;
  ba::steady_timer deadline_;
  uint8_t is_opened;


};
}

#endif
