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
  ser_req.http_version = cli_req.http_version;

  for (const Header& h : cli_req.headers) {
    ser_req.headers.push_back(h);
  }
}


void RequestHandler::ComposeRequest(const Request& req, std::string& str_req) {
  str_req.clear();
  str_req += req.method;
  str_req.push_back(' ');
  str_req += req.uri;
  str_req.push_back(' ');
  str_req += req.http_version;
  str_req += "\r\n";
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
