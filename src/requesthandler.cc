#include "requesthandler.h"

#include <iostream>
#include <algorithm>
#include <sstream>
#include <string>
#include <list>

#include "request.h"
#include "filter.h"
#include "connhandler.h"
#include "conf.h"

namespace msystem {
RequestHandler::RequestHandler(ConnHandler& conn_handler)
    : conn_handler_(conn_handler){

}

int RequestHandler::ProcessRequest() {
  std::size_t p;
  ConfigPool* config_pool = ConfigPool::GetConfigPool();

  if (conn_handler_.GetRequest().http_version.find_first_of("HTTP/") == 0) {
    std::string& version = conn_handler_.GetRequest().http_version;
    p = version.find_first_of('.');
    if (p == std::string::npos)
      return -1;
    try {
      conn_handler_.GetConnInfo().protocol.major = std::atoi(version.substr(5, p-5).c_str());
      conn_handler_.GetConnInfo().protocol.minor = std::atoi(version.substr(p+1).c_str());
    }  catch (...) {
      return -1;
    }
  } else {
    return -1;
  }

  conn_handler_.GetRequest().raw_url = conn_handler_.GetRequest().url;
  if (conn_handler_.GetRequest().url.find_first_of("http://") == 0 ||
      conn_handler_.GetRequest().url.find_first_of("ftp://") == 0) {
    std::size_t skiped_type_p = conn_handler_.GetRequest().url.find_first_of("//") + 2;
    conn_handler_.GetRequest().url = conn_handler_.GetRequest().url.substr(skiped_type_p);
    ExtractUrl(conn_handler_.GetRequest().url, HTTP_PORT);
  } else if (conn_handler_.GetRequest().method == "CONNECT") {
    ExtractUrl(conn_handler_.GetRequest().url, HTTP_PORT_SSL);
    //need check allowed connect ports
    conn_handler_.GetConnInfo().connect_method = true;
  } else {
    return -1;
  }

  //过滤
  Filter* filter = Filter::GetFilter();
  if (filter) {
    if (!config_pool->filter_url&&!filter->FilterByDomain(conn_handler_.GetRequest().host)) {
      return -2;
    } else if (config_pool->filter_url&&!filter->FilterByUrl(conn_handler_.GetRequest().url)) {
      return -2;
    }
  }

  if (config_pool->bindsame) {
    auto client_endpoint = conn_handler_.GetConnInfo().client_socket.remote_endpoint();
    conn_handler_.GetRequest().host = client_endpoint.address().to_string();
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
  if (conn_handler_.GetConnInfo().connect_method) {
    return;
  }
  RemoveConnectionHeaders();
  for (i = 0; i != (sizeof (skip_headers) / sizeof (char *)); i++) {
    Header h;
    h.name = skip_headers[i];
    auto iter = std::find(conn_handler_.GetRequest().headers.begin(), conn_handler_.GetRequest().headers.end(), h);
    conn_handler_.GetRequest().headers.erase(iter);
  }
}


void RequestHandler::ComposeRequest(const Request& req, std::string& str_req) {
  str_req.clear();
  str_req += req.method;
  str_req.push_back(' ');
  str_req += req.raw_url;
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

int RequestHandler::ExtractUrl(const std::string& url, int default_port) {
  std::size_t p;

  p = url.find_first_of('@');
  if (p == std::string::npos) {
    conn_handler_.GetRequest().host = url;
    conn_handler_.GetRequest().url = "/";
  } else {
    conn_handler_.GetRequest().host = conn_handler_.GetRequest().url.substr(0, p);
    conn_handler_.GetRequest().url = conn_handler_.GetRequest().url.substr(p);
  }
  StripUserNameAndPassword(conn_handler_.GetRequest().host);

  conn_handler_.GetRequest().port = StripReturnPort(conn_handler_.GetRequest().host);
  conn_handler_.GetRequest().port = (conn_handler_.GetRequest().port != 0) ? conn_handler_.GetRequest().port : default_port;

  //remove ipv6 surrounding '[' and ']'
  p = conn_handler_.GetRequest().host.find_first_of(']');
  if (p != std::string::npos && conn_handler_.GetRequest().host[0] == '[')
    conn_handler_.GetRequest().host = conn_handler_.GetRequest().host.substr(1, conn_handler_.GetRequest().host.length() - 2);

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
    Header h;
    h.name = headers[i];
    auto iter = std::find(conn_handler_.GetRequest().headers.begin(), conn_handler_.GetRequest().headers.end(), h);
    if (iter == conn_handler_.GetRequest().headers.end())
      continue;
    int len = h.value.length();
    ptr = h.value.c_str();
    while ((ptr = std::strpbrk(iter->value.c_str(), "()<>@,;:\\\"/[]?={} \t")))
      h.value[ptr - h.value.c_str()] = '\0';
    ptr = h.value.c_str();
    while (ptr < h.value.c_str() + len) {
      Header h1;
      h1.name = ptr;
      auto iter1 = std::find(conn_handler_.GetRequest().headers.begin(), conn_handler_.GetRequest().headers.end(), h);
      if (iter1 != conn_handler_.GetRequest().headers.end())
        conn_handler_.GetRequest().headers.erase(iter1);
      ptr += std::strlen(ptr)+1;
      while (ptr < h.value.c_str() + len && *ptr == '\0')
        ptr++;
    }
    conn_handler_.GetRequest().headers.erase(iter);
  }
}
}
