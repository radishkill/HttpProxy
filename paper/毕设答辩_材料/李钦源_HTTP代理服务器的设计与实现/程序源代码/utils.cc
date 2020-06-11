#include "utils.h"

namespace msystem {

std::tuple<std::string, int> Utils::SpliteHost(const std::string &host) {
  std::size_t p;
  std::string host_addr;
  int host_port;
  if ((p=host.find(':')) == std::string::npos) {
    return std::make_tuple(host, 0);
  } else {
    try {
      host_port = static_cast<uint16_t>(std::stoi(host.substr(p+1)));
    }  catch (...) {
      host_port = 0;
    }
    host_addr = host.substr(0, p);
  }
  return std::make_tuple(host_addr, host_port);
}

}
