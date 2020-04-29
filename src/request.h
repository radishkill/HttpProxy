#ifndef MSYSTEM_REQUEST_H
#define MSYSTEM_REQUEST_H

#include <string>
#include <vector>

#define HTTP_PORT 80
#define HTTP_PORT_SSL 443

namespace msystem {
struct Header {
  std::string name;
  std::string value;
  bool operator==(const Header& h) const {
    return this->name==h.name?true:false;
  }
};

struct Request {
  std::string method;
  std::string raw_url;
  std::string url;
  std::string http_version;
  std::string host;
  int port;
  std::vector<Header> headers;
};
}

#endif
