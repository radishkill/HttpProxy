#include "connhandler.h"

#include <iostream>

#include <boost/asio.hpp>
#include <boost/asio/steady_timer.hpp>

#include "conf.h"
#include "connmanager.h"

namespace msystem {


namespace asio = boost::asio;

ConnHandler::ConnHandler(asio::ip::tcp::socket socket,
    ConnManager& manager)
    : conn_info_(std::move(socket)),
      conn_manager_(manager),
      deadline_(socket.get_executor()),
      stopped_(false) {
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
  deadline_.expires_after(asio::chrono::seconds(config_pool_->idletimeout));
  deadline_.async_wait(std::bind(&ConnHandler::CheckDeadline, this));
  conn_info_.server_socket.async_connect(server_endpoint,
      [this, self](const boost::system::error_code& ec) {
        if (!ec) {
          conn_info_.state_ = ConnInfo::kConnToServer;
          if (from_cli_request_.method == "CONNECT") {
            conn_info_.connect_method = true;
            std::string str_response;
            RequestHandler::GetSslResponse(str_response);
            cli_send_buffer_.clear();
            cli_send_buffer_.push_back(boost::asio::buffer(str_response));
            WriteToClient();
            ReadFromServer();
          } else {
            std::string str_req;
            request_handler_.HandleRequest(from_cli_request_, to_ser_request_);
            RequestHandler::ComposeRequest(to_ser_request_, str_req);
            ser_send_buffer_.clear();
            ser_send_buffer_.push_back(boost::asio::buffer(str_req));
            WriteToServer();
            ReadFromServer();
          }
        } else {
          std::cout << ec.message() << "\n";
          conn_manager_.Stop(self);
        }
      });
}

void ConnHandler::WriteToServer() {
  auto self = shared_from_this();
  conn_info_.server_socket.async_write_some(ser_send_buffer_,
      [this, self](const boost::system::error_code& ec, std::size_t bytes_transferred) {
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
      [this, self](const boost::system::error_code& ec, std::size_t bytes_transferred) {
          if (!ec) {
            cli_send_buffer_.clear();
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
      [this, self](const boost::system::error_code& ec, std::size_t) {
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
      [this, self](const boost::system::error_code& ec, std::size_t bytes_transferred) {
        std::cout << "[tid:" << std::this_thread::get_id() << "] \n";
        if (!ec) {
          if (conn_info_.state_ == ConnInfo::kConnInitial) {
            RequestParser::ResultType result;
            std::tie(result, std::ignore) = request_parser_.parse(
                from_cli_request_, cli_recv_buffer_.data(), cli_recv_buffer_.data() + bytes_transferred);
            if (result == RequestParser::kGood) {
              //debug
              std::cout << "url: " << to_ser_request_.uri << "\n";
              std::cout << "method: " << to_ser_request_.method << "\n";
              std::cout << "version: " << to_ser_request_.http_version << "\n";
              for (auto header : to_ser_request_.headers) {
                std::cout << header.name << ":" << header.value << "\n";
              }
              //get server host and port
              std::string address;
              uint16_t port;
              Header header;
              header.name = "Host";
              auto iter = std::find(from_cli_request_.headers.begin(), from_cli_request_.headers.end(), header);
              if (iter == from_cli_request_.headers.end()) {
                conn_info_.client_socket.close();
                return;
              }
              RequestHandler::UrlGetHostAndPort(iter->value, address, port);
              if (config_pool_->bindsame) {
                auto client_endpoint = conn_info_.client_socket.remote_endpoint();
                address = client_endpoint.address().to_string();
              }
              conn_info_.state_ = ConnInfo::kRequestFromClient;
              ConnToServer(address, port);
              ReadFromClient();
            } else if (result == RequestParser::kBad) {
              //reply_ = reply::stock_reply(reply::bad_request);
              //do_write();
            } else {
              ReadFromClient();
            }
          } else {
            ser_send_buffer_.clear();
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

void ConnHandler::CheckDeadline() {
  auto self = shared_from_this();
  if (stopped_)
    return;
  //超时
  if (deadline_.expiry() <= asio::steady_timer::clock_type::now()) {
    if (conn_info_.state_ == ConnInfo::kRequestFromClient) {
      conn_manager_.Stop(self);
    }
    deadline_.expires_at(asio::steady_timer::time_point::max());
  }
  deadline_.async_wait(std::bind(&ConnHandler::CheckDeadline, this));
}

void ConnHandler::Stop() {
  if (stopped_)
    return;
  stopped_ = true;
  conn_info_.client_socket.close();
  if (conn_info_.state_ == ConnInfo::kConnInitial || conn_info_.state_ == ConnInfo::kRequestFromClient)
    conn_info_.server_socket.close();
}

ConnHandler::~ConnHandler() {

}

}
