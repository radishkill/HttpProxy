#include "connhandler.h"

#include <boost/asio.hpp>

#include <iostream>

#include "conf.h"
#include "connmanager.h"

namespace msystem {

namespace asio = boost::asio;

ConnHandler::ConnHandler(asio::ip::tcp::socket socket,
    ConnManager& manager)
    : conn_info_(std::move(socket)),
      conn_manager_(manager) {
  this->config_pool_ = ConfigPool::GetConfigPool();
}

void ConnHandler::Run() {
  auto remote_endpoint = conn_info_.client_socket.remote_endpoint();
  std::cout << "get connect from\n";
  std::cout << remote_endpoint.address().to_string() << ":"
      << remote_endpoint.port() << "\n";

  this->ReadFromClient();
}

void ConnHandler::ConnToServer(std::string& host, uint16_t& port) {
  auto self = shared_from_this();
  asio::ip::tcp::resolver resolver(conn_info_.server_socket.get_executor());
  asio::ip::tcp::endpoint server_endpoint = *resolver.resolve(host, std::to_string(port)).begin();
  conn_info_.server_socket.async_connect(server_endpoint,
      [this, self](boost::system::error_code ec) {
        if (!ec) {
          conn_info_.state_ = ConnInfo::kConnToServer;
          if (from_cli_request_.method == "CONNECT") {
            conn_info_.connect_method = true;
            std::string str_req;
            str_req += "HTTP/1.0 200 Connection established\r\n";
            str_req += "Proxy-agent: tinyproxy/1.10.0\r\n\r\n";
            cli_send_buffer_.erase(cli_send_buffer_.begin(), cli_send_buffer_.end());
            cli_send_buffer_.push_back(boost::asio::buffer(str_req));
            WriteToClient();
            ReadFromServer();
          } else {
            std::string str_req;
            RequestParser::ComposeRequest(to_ser_request_, str_req);
            ser_send_buffer_.erase(ser_send_buffer_.begin(), ser_send_buffer_.end());
            ser_send_buffer_.push_back(boost::asio::buffer(str_req));
            WriteToServer();
            ReadFromServer();
          }
        } else {
          std::cout << ec.message() << "\n";
          conn_info_.client_socket.close();
        }
      });
}

void ConnHandler::WriteToServer() {
  auto self = shared_from_this();
  conn_info_.server_socket.async_write_some(ser_send_buffer_,
      [this, self](boost::system::error_code ec, std::size_t bytes_transferred) {
          if (!ec) {
            if (conn_info_.state_ == ConnInfo::kConnToServer) {
              conn_info_.state_ = ConnInfo::kProxySuccess;
            }
          } else {
            std::cout << ec.message() << "\n";
            conn_manager_.Stop(self);
          }
      });
}

void ConnHandler::ReadFromServer() {
  auto self = shared_from_this();
  conn_info_.server_socket.async_read_some(asio::buffer(ser_recv_buffer_),
      [this, self](boost::system::error_code ec, std::size_t bytes_transferred) {
          if (!ec) {
            cli_send_buffer_.erase(cli_send_buffer_.begin(), cli_send_buffer_.end());
            cli_send_buffer_.push_back(boost::asio::buffer(ser_recv_buffer_, bytes_transferred));
            WriteToClient();
            ReadFromServer();
          } else {
            std::cout << ec.message() << "\n";
            conn_manager_.Stop(self);
          }
      });
}

void ConnHandler::WriteToClient() {
  auto self = shared_from_this();
  conn_info_.client_socket.async_write_some(cli_send_buffer_,
      [this, self](boost::system::error_code ec, std::size_t) {
          if (!ec) {
            if (conn_info_.state_ == ConnInfo::kConnToServer) {
              conn_info_.state_ = ConnInfo::kProxySuccess;
            }
          } else {
            std::cout << ec.message() << "\n";
            conn_manager_.Stop(self);
          }
      });
}

void ConnHandler::ReadFromClient() {
  auto self = shared_from_this();
  conn_info_.client_socket.async_receive(asio::buffer(cli_recv_buffer_),
      [this, self](boost::system::error_code ec, std::size_t bytes_transferred) {
        std::cout << "[tid:" << std::this_thread::get_id() << "] \n";
        if (!ec) {
          if (conn_info_.state_ == ConnInfo::kConnInitial) {
            RequestParser::ResultType result;
            std::tie(result, std::ignore) = request_parser_.parse(
                from_cli_request_, cli_recv_buffer_.data(), cli_recv_buffer_.data() + bytes_transferred);
            if (result == RequestParser::kGood) {
              request_handler_.HandleRequest(from_cli_request_, to_ser_request_);
              std::cout << "url: " << to_ser_request_.uri << "\n";
              std::cout << "method: " << to_ser_request_.method << "\n";
              std::cout << "version: " << to_ser_request_.http_version_major << ".";
              std::cout << to_ser_request_.http_version_minor << "\n";
              for (auto header : to_ser_request_.headers) {
                std::cout << header.name << ":" << header.value << "\n";
              }
              std::string host;
              uint16_t port;
              Header header;
              header.name = "Host";
              auto iter = std::find(from_cli_request_.headers.begin(), from_cli_request_.headers.end(), header);
              if (iter == from_cli_request_.headers.end()) {
                conn_info_.client_socket.close();
                return;
              }
              RequestParser::UrlGetHostAndPort(iter->value, host, port);
              conn_info_.state_ = ConnInfo::kRequestFromClient;
              ConnToServer(host, port);
              ReadFromClient();
            } else if (result == RequestParser::kBad) {
              //reply_ = reply::stock_reply(reply::bad_request);
              //do_write();
            } else {
              ReadFromClient();
            }
          } else {
            ser_send_buffer_.erase(ser_send_buffer_.begin(), ser_send_buffer_.end());
            ser_send_buffer_.push_back(boost::asio::buffer(cli_recv_buffer_, bytes_transferred));
            WriteToServer();
            ReadFromClient();
          }
        } else {
          std::cout << ec.message() << "\n";
          conn_manager_.Stop(self);
        }
  });
}

void ConnHandler::Stop() {
  conn_info_.client_socket.close();
  if (conn_info_.state_ == ConnInfo::kConnInitial || conn_info_.state_ == ConnInfo::kRequestFromClient)
    conn_info_.server_socket.close();
}

ConnHandler::~ConnHandler() {

}

}
