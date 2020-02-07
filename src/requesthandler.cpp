#include "requesthandler.h"

#include <iostream>
#include <algorithm>
#include <sstream>

#include "request.h"

namespace msystem {

RequestHandler::RequestHandler() {
}

void RequestHandler::HandleRequest(const Request &cli_req, Request &ser_req) {
  Header header;
  header.name = "authorization";

  auto iter = std::find(cli_req.headers.begin(), cli_req.headers.end(), header);
  if (iter != cli_req.headers.end()) {

  }
  ser_req.uri = cli_req.uri;
  ser_req.method = cli_req.method;
  for (const Header& h : cli_req.headers) {
    ser_req.headers.push_back(h);
  }
}

void RequestHandler::UrlGetHostAndPort(const std::string& val, std::string& address, uint16_t& port) {
  address.clear();
  std::size_t p;
  if ((p=val.find(':')) == std::string::npos) {
    port = 80;
  } else {
    port = static_cast<uint16_t>(std::stoi(val.substr(p+1)));
    address = val.substr(0, p);
  }
}

void RequestHandler::GetSslResponse(std::string& str_response) {
  str_response.clear();
  str_response += "HTTP/1.0 200 Connection established\r\n";
  str_response += "Proxy-agent: httpproxy/1.0\r\n\r\n";
}

void RequestHandler::ComposeRequest(const Request& req, std::string& str_req) {
  str_req.clear();
  str_req += req.method;
  str_req += " / HTTP/1.1\r\n";
  for (const Header &h : req.headers) {
    str_req += h.name;
    str_req += ": ";
    str_req += h.value;
    str_req += "\r\n";
  }
  str_req += "\r\n";
}


RequestHandler::~RequestHandler() {

}

}
