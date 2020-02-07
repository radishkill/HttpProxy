#ifndef CONNHANDLER_H
#define CONNHANDLER_H

#include "conninfo.h"
#include "request.h"
#include "requestparser.h"
#include "requesthandler.h"


namespace msystem {

class ConfigPool;
class ConnManager;

class ConnHandler : public std::enable_shared_from_this<ConnHandler>  {
 public:
  ConnHandler(boost::asio::ip::tcp::socket socket, ConnManager& manager);
  void Run();
  void ConnToServer(std::string& host, uint16_t& port);
  void WriteToServer();
  void ReadFromServer();
  void WriteToClient();
  void ReadFromClient();
  void CheckDeadline();
  void Stop();
  ~ConnHandler();
 private:
  ConnInfo conn_info_;
  ConfigPool* config_pool_;
  std::array<char, 8129> cli_recv_buffer_;
  std::array<char, 8129> ser_recv_buffer_;
  std::vector<boost::asio::const_buffer> cli_send_buffer_;
  std::vector<boost::asio::const_buffer> ser_send_buffer_;
  Request from_cli_request_;
  Request to_ser_request_;
  RequestParser request_parser_;
  RequestHandler request_handler_;
  ConnManager& conn_manager_;
  asio::steady_timer deadline_;
  uint8_t stopped_;
};
}

#endif
