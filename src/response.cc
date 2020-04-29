#include "response.h"



namespace msystem {
void Response::GetSslResponse(std::string& str_response) {
  str_response.clear();
  str_response += "HTTP/1.1 200 Connection established\r\n";
  str_response += "Proxy-agent: httpproxy/1.0\r\n\r\n";
}

void Response::EstablishHttpConnection(Request& req, std::string &str_response) {
  str_response = req.method;
  str_response += ' ';
  str_response += req.raw_url;
  str_response += " HTTP/1.0\r\n";
  str_response += "Host: ";
  str_response += req.host;
  str_response += ':';
  str_response += std::to_string(req.port);
  str_response += "\r\n";
  str_response += "Connection: close\r\n\r\n";
}

void Response::ExpectationFailed(std::string& str_response) {
  str_response.clear();
  str_response += "HTTP/1.1 417 Expectation Failedr\r\n";
  str_response += "Proxy-agent: httpproxy/1.0\r\n\r\n";
}
void Response::BadRequest(std::string& str_response) {
  str_response.clear();
  str_response += "HTTP/1.1 400 Bad Request\r\n";
  str_response += "Proxy-agent: httpproxy/1.0\r\n\r\n";
}
}
