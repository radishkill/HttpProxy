#ifndef MSYSTEM_REQUEST_H
#define MSYSTEM_REQUEST_H

#include <string>
#include <vector>

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
  std::string uri;
  int http_version_major;
  int http_version_minor;
  std::vector<Header> headers;
};
}

#endif
