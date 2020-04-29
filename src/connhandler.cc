#include "connhandler.h"

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

ConnHandler::ConnHandler(ConnManager& manager)
  : conn_info_(io_ctx_),
    request_parser_(req_),
    request_handler_(*this),
    conn_manager_(manager),
    deadline_(io_ctx_),
    stopped_(false),
    stopping_(false) {
  this->config_pool_ = ConfigPool::GetConfigPool();
}

void ConnHandler::Run() {
//  auto remote_endpoint = conn_info_.client_socket.remote_endpoint();
//  std::cout << "get connect from\n";
//  std::cout << remote_endpoint.address().to_string() << ":"
//            << remote_endpoint.port() << "\n";
//  std::cout << "thread_id:" << std::this_thread::get_id() << std::endl;
  if (ReadRequest() == -1) {
    return;
  }
  //deadline_.async_wait(std::bind(&ConnHandler::CheckDeadline, this));
}

void ConnHandler::ConnectToServer() {
  auto self = shared_from_this();
  asio::ip::tcp::resolver resolver(conn_info_.server_socket.get_executor());
  asio::ip::tcp::endpoint server_endpoint;
  try {
    server_endpoint = *resolver.resolve(req_.host, std::to_string(req_.port)).begin();
  }  catch (...) {
    std::string str_response;
    Response::ExpectationFailed(str_response);
    cli_send_buffer_.push_back(asio::buffer(str_response));
    conn_info_.client_socket.write_some(asio::buffer(cli_send_buffer_));
    conn_manager_.Stop(self);
    return;
  }
  deadline_.expires_after(std::chrono::seconds(config_pool_->idletimeout));
  conn_info_.server_socket.async_connect(server_endpoint, [&](const boost::system::error_code& ec) {
    if (ec) {
      std::cout << ec.value() << std::endl;
      std::string str_response;
      Response::ExpectationFailed(str_response);
      cli_send_buffer_.push_back(asio::buffer(str_response));
      //conn_info_.client_socket.write_some(asio::buffer(cli_send_buffer_));
      //conn_manager_.Stop(self);
      return ;
    }
    conn_info_.state_ = ConnInfo::kConnToServer;
    if (conn_info_.connect_method) {
      std::string str_response;
      Response::GetSslResponse(str_response);
      cli_send_buffer_.clear();
      cli_send_buffer_.push_back(boost::asio::buffer(str_response));
      WriteToClient();
      Response::EstablishHttpConnection(req_, str_response);
      WriteToServer();
      ReadFromClient();
      ReadFromServer();
    } else {
      std::string str_req;
      RequestHandler::ComposeRequest(req_, str_req);
      ser_send_buffer_.clear();
      ser_send_buffer_.push_back(boost::asio::buffer(str_req));
      WriteToServer();
      ReadFromServer();
      ReadFromClient();
    }
  });
}

void ConnHandler::WriteToServer() {
  std::lock_guard<std::mutex> lock(mutex_);
  if (stopped_)
    return;
  auto self = shared_from_this();
  conn_info_.server_socket.async_write_some(ser_send_buffer_,
      [this, self](const boost::system::error_code& ec, std::size_t bytes_transferred) {
    if (ec) {
      std::cout << ec.message() << "\n";
      std::string str_response;
      Response::ExpectationFailed(str_response);
      cli_send_buffer_.push_back(asio::buffer(str_response));
      //conn_info_.client_socket.write_some(asio::buffer(cli_send_buffer_));
      conn_manager_.Stop(self);
    }
    if (conn_info_.state_ == ConnInfo::kConnToServer)
      conn_info_.state_ = ConnInfo::kProxySuccess;
  });
}

void ConnHandler::ReadFromServer() {
  std::lock_guard<std::mutex> lock(mutex_);
  if (stopped_)
    return;
  auto self = shared_from_this();
  conn_info_.server_socket.async_read_some(asio::buffer(ser_recv_buffer_),
                                           [this, self](const boost::system::error_code& ec, std::size_t bytes_transferred) {
    if (ec) {
      std::cout << ec.message() << "\n";
      std::string str_response;
      Response::ExpectationFailed(str_response);
      cli_send_buffer_.push_back(asio::buffer(str_response));
      //conn_info_.client_socket.write_some(asio::buffer(cli_send_buffer_));
      conn_manager_.Stop(self);
      return ;
    }
    cli_send_buffer_.clear();
    cli_send_buffer_.push_back(boost::asio::buffer(ser_recv_buffer_, bytes_transferred));
    WriteToClient();
    ReadFromServer();
  });
}

void ConnHandler::WriteToClient() {
  std::lock_guard<std::mutex> lock(mutex_);
  if (stopped_)
    return;
  auto self = shared_from_this();
  conn_info_.client_socket.async_write_some(cli_send_buffer_,
                                            [this, self](const boost::system::error_code& ec, std::size_t) {
    if (ec) {
      std::cout << ec.message() << "\n";
      std::string str_response;
      Response::ExpectationFailed(str_response);
      cli_send_buffer_.push_back(asio::buffer(str_response));
      //conn_info_.client_socket.write_some(asio::buffer(cli_send_buffer_));
      conn_manager_.Stop(self);
      return;
    }
    if (conn_info_.state_ == ConnInfo::kConnToServer) {
      conn_info_.state_ = ConnInfo::kProxySuccess;
    }
  });
}

void ConnHandler::ReadFromClient() {
  std::lock_guard<std::mutex> lock(mutex_);
  if (stopped_)
    return;
  auto self = shared_from_this();
  conn_info_.client_socket.async_receive(asio::buffer(cli_recv_buffer_),
                                         [this, self](const boost::system::error_code& ec, std::size_t bytes_transferred) {
    if (ec) {
      std::cout << ec.message() << "\n";
      std::string str_response;
      Response::ExpectationFailed(str_response);
      cli_send_buffer_.push_back(asio::buffer(str_response));
      //conn_info_.client_socket.write_some(asio::buffer(cli_send_buffer_));
      conn_manager_.Stop(self);
      return ;
    }
    ser_send_buffer_.clear();
    ser_send_buffer_.push_back(boost::asio::buffer(cli_recv_buffer_, bytes_transferred));
    WriteToServer();
    ReadFromClient();

  });
}

void ConnHandler::CheckDeadline() {
  if (stopped_)
    return;
  auto self = shared_from_this();
  //超时
  if (deadline_.expiry() <= asio::steady_timer::clock_type::now()) {
    if (conn_info_.state_ == ConnInfo::kRequestFromClient) {
      std::cout << "connect time out" << std::endl;
      conn_manager_.Stop(self);
      return;
    }
    deadline_.expires_at(asio::steady_timer::time_point::max());
  }
  deadline_.async_wait(std::bind(&ConnHandler::CheckDeadline, this));
}

void ConnHandler::Stop() {
  std::lock_guard<std::mutex> lock(mutex_);
  if (stopped_)
    return;
  stopped_ = true;
  conn_info_.client_socket.close();
  if (conn_info_.state_ != ConnInfo::kConnInitial && conn_info_.state_ != ConnInfo::kRequestFromClient)
    conn_info_.server_socket.close();
}

ConnHandler::~ConnHandler() {

}

int ConnHandler::ReadRequest() {
  auto self = shared_from_this();
  boost::system::error_code ec;
  while (true) {
    conn_info_.client_socket.async_receive(asio::buffer(cli_recv_buffer_),
        [this, self](const boost::system::error_code& ec, std::size_t bytes_transferred) {
      if (ec) {
        std::cout << ec.message() << "\n";
        std::string str_response;
        Response::ExpectationFailed(str_response);
        cli_send_buffer_.push_back(asio::buffer(str_response));
        //conn_info_.client_socket.write_some(asio::buffer(cli_send_buffer_));
        conn_manager_.Stop(self);
        return ;
      }
      ser_send_buffer_.clear();
      ser_send_buffer_.push_back(boost::asio::buffer(cli_recv_buffer_, bytes_transferred));
      WriteToServer();
      ReadFromClient();

    });
    auto bytes_transferred = conn_info_.client_socket.read_some(asio::buffer(cli_recv_buffer_), ec);
    if (ec) {
      std::cout << ec.message() << "\n";
      conn_manager_.Stop(self);
      return -1;
    }
    RequestParser::ResultType result;
    std::tie(result, std::ignore) = request_parser_.Parse(cli_recv_buffer_.data(), cli_recv_buffer_.data() + bytes_transferred);
    if (result == RequestParser::kGood) {
      //debug
      std::cout << "url: " << req_.url << "\n";
      std::cout << "method: " << req_.method << "\n";
      std::cout << "version: " << req_.http_version << "\n";
      for (auto header : req_.headers) {
        std::cout << header.name << ":" << header.value << "\n";
      }
      break;
    } else if (result == RequestParser::kBad) {
      std::string str_response;
      Response::ExpectationFailed(str_response);
      cli_send_buffer_.push_back(asio::buffer(str_response));
      conn_info_.client_socket.write_some(asio::buffer(cli_send_buffer_), ec);
      conn_manager_.Stop(self);
      return 0;
    }
  }
  if (request_handler_.ProcessRequest() < 0) {
    std::string str_response;
    Response::ExpectationFailed(str_response);
    cli_send_buffer_.push_back(asio::buffer(str_response));
    conn_info_.client_socket.write_some(asio::buffer(cli_send_buffer_), ec);
    conn_manager_.Stop(self);
    return 0;
  }
  conn_info_.state_ = ConnInfo::kRequestFromClient;
  ConnectToServer();
  return 0;
}

}
