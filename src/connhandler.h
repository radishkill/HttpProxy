#ifndef CONNHANDLER_H
#define CONNHANDLER_H

#include "condition_variable"
#include "mutex"

#include "conninfo.h"
#include "request.h"
#include "requestparser.h"
#include "requesthandler.h"

namespace msystem {

class ConfigPool;
class ConnManager;

class ConnHandler : public std::enable_shared_from_this<ConnHandler>  {
 public:
  ConnHandler(ConnManager& manager);
  void Run();
  void ConnectToServer();
  void WriteToServer();
  void ReadFromServer();
  void WriteToClient();
  void ReadFromClient();
  void CheckDeadline();
  ConnInfo& GetConnInfo() {
    return conn_info_;
  }
  Request& GetRequest() {
    return req_;
  }
  RequestParser& GetRequestParser() {
    return request_parser_;
  }
  RequestHandler& GetRequestHandler() {
    return request_handler_;
  }
  void Stop();
  ~ConnHandler();
 private:
  int ReadRequest();
  boost::asio::io_context io_ctx_;
  ConnInfo conn_info_;
  ConfigPool* config_pool_;
  std::array<char, 8129> cli_recv_buffer_;
  std::array<char, 8129> ser_recv_buffer_;
  std::vector<asio::const_buffer> cli_send_buffer_;
  std::vector<asio::const_buffer> ser_send_buffer_;
  Request req_;
  RequestParser request_parser_;
  RequestHandler request_handler_;
  ConnManager& conn_manager_;
  asio::steady_timer deadline_;
  uint8_t stopped_;
  uint8_t stopping_;
  mutable std::mutex mutex_;
  std::condition_variable condition_;
};
}

#endif
