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

RequestHandler::~RequestHandler() {

}

}
