#include "requesthandler.h"

#include <iostream>
#include <algorithm>
#include <sstream>
#include <string>
#include <list>

#include "filter.h"
#include "proxyserver.h"
#include "conf.h"

namespace msystem {
RequestHandler::RequestHandler(Connection& conn)
    : conn_(conn) {

}

int RequestHandler::ProcessRequest() {
  std::size_t p;
  ConfigPool* config_pool = ConfigPool::GetConfigPool();

  if (conn_.GetHttpProtocol().http_version.find_first_of("HTTP/") == 0) {
    std::string& version = conn_.GetHttpProtocol().http_version;
    p = version.find_first_of('.');
    if (p == std::string::npos)
      return -1;
    try {
      conn_.GetHttpProtocol().protocol_major = std::atoi(version.substr(5, p-5).c_str());
      conn_.GetHttpProtocol().protocol_minor = std::atoi(version.substr(p+1).c_str());
    }  catch (...) {
      return -1;
    }
  } else {
    return -1;
  }

  if (conn_.GetHttpProtocol().raw_url.find_first_of("http://") == 0 ||
      conn_.GetHttpProtocol().raw_url.find_first_of("ftp://") == 0) {
    std::size_t skiped_type_p = conn_.GetHttpProtocol().raw_url.find_first_of("//") + 2;
    conn_.GetHttpProtocol().url = conn_.GetHttpProtocol().raw_url.substr(skiped_type_p);
    ExtractUrl(conn_.GetHttpProtocol().url, HTTP_PORT);
    conn_.GetHttpProtocol().connect_method = false;
  } else if (conn_.GetHttpProtocol().method == "CONNECT") {
    if (conn_.GetHttpProtocol().raw_url.find_first_of("https://") == 0) {
      std::size_t skiped_type_p = conn_.GetHttpProtocol().raw_url.find_first_of("//") + 2;
      conn_.GetHttpProtocol().url = conn_.GetHttpProtocol().raw_url.substr(skiped_type_p);
    } else {
      conn_.GetHttpProtocol().url = conn_.GetHttpProtocol().raw_url;
    }
    ExtractUrl(conn_.GetHttpProtocol().url, HTTP_PORT_SSL);
    //need check allowed connect ports
    conn_.GetHttpProtocol().connect_method = true;
  } else {
    ExtractUrl(conn_.GetHttpProtocol().headers.find("host")->second, HTTP_PORT);
    conn_.GetHttpProtocol().connect_method = false;
  }
  std::cout << "true host : " << conn_.GetHttpProtocol().host << std::endl;

  //过滤
  Filter* filter = Filter::GetFilter();
  if (filter) {
    if (!config_pool->filter_url&&!filter->FilterByDomain(conn_.GetHttpProtocol().host)) {
      return -2;
    } else if (config_pool->filter_url&&!filter->FilterByUrl(conn_.GetHttpProtocol().url)) {
      return -2;
    }
  }

  if (config_pool->bindsame) {
    auto client_endpoint = conn_.Socket().remote_endpoint();
    conn_.GetHttpProtocol().host = client_endpoint.address().to_string();
  } else if (!config_pool->upstream.empty()) {
    //std::tie(bind_host, bind_port) = Utils::SpliteHost(config_pool->upstream);
  }
  return 0;
}

void RequestHandler::ProcessClientHeaders() {
  static const char* const skip_headers[] = {
    "host",
    "keep-alive",
    "proxy-connection",
    "te",
    "trailers",
    "upgrade"
  };
  int i;
  if (conn_.GetHttpProtocol().connect_method) {
    return;
  }
  RemoveConnectionHeaders();
  for (i = 0; i != (sizeof (skip_headers) / sizeof (char *)); i++) {
    conn_.GetHttpProtocol().headers.erase(skip_headers[i]);
  }
}


RequestHandler::~RequestHandler() {

}

int RequestHandler::ExtractUrl(const std::string& url, int default_port) {
  std::size_t p;

  p = url.find_first_of('/');
  if (p == std::string::npos) {
    conn_.GetHttpProtocol().host = url;
    conn_.GetHttpProtocol().url = "/";
  } else {
    conn_.GetHttpProtocol().host = conn_.GetHttpProtocol().url.substr(0, p);
    conn_.GetHttpProtocol().url = conn_.GetHttpProtocol().url.substr(p);
  }
  StripUserNameAndPassword(conn_.GetHttpProtocol().host);

  conn_.GetHttpProtocol().port = StripReturnPort(conn_.GetHttpProtocol().host);
  conn_.GetHttpProtocol().port = (conn_.GetHttpProtocol().port != 0) ? conn_.GetHttpProtocol().port : default_port;

  //remove ipv6 surrounding '[' and ']'
  p = conn_.GetHttpProtocol().host.find_first_of(']');
  if (p != std::string::npos && conn_.GetHttpProtocol().host[0] == '[')
    conn_.GetHttpProtocol().host = conn_.GetHttpProtocol().host.substr(1, conn_.GetHttpProtocol().host.length() - 2);

  return 0;
}

void RequestHandler::StripUserNameAndPassword(std::string& host) {
  std::size_t p;
  p = host.find_first_of('@');
  if (p == std::string::npos)
    return;
  host = host.substr(0, p);
}

int RequestHandler::StripReturnPort(std::string& host) {
  std::size_t p;
  int port;
  /* 检查ipv6 */
  p = host.find_first_of(']');
  if (p != std::string::npos)
    return 0;

  p = host.find_first_of(':');
  if (p == std::string::npos)
    return 0;
  ++p;
  try {
    port = std::atoi(host.substr(p).c_str());
  }  catch (...) {
    return 0;
  }
  --p;
  host = host.substr(0, p);
  return port;
}

void RequestHandler::RemoveConnectionHeaders() {
  static const char* const headers[] = {
    "connection",
    "proxy-connection"
  };
  int i;
  const char* ptr;
  for (i = 0; i != sizeof (headers) / sizeof (char *); i++) {
    auto iter = conn_.GetHttpProtocol().headers.find(headers[i]);
    if (iter == conn_.GetHttpProtocol().headers.end())
      continue;
    int len = iter->second.length();
    ptr = iter->second.c_str();
    while ((ptr = std::strpbrk(iter->second.c_str(), "()<>@,;:\\\"/[]?={} \t")))
      iter->second[ptr - iter->second.c_str()] = '\0';
    ptr = iter->second.c_str();
    while (ptr < iter->second.c_str() + len) {

      auto iter1 = conn_.GetHttpProtocol().headers.find(std::string(ptr));
      if (iter1 != conn_.GetHttpProtocol().headers.end())
        conn_.GetHttpProtocol().headers.erase(iter1);
      ptr += std::strlen(ptr)+1;
      while (ptr < iter->second.c_str() + len && *ptr == '\0')
        ptr++;
    }
    conn_.GetHttpProtocol().headers.erase(iter);
  }
}
}
