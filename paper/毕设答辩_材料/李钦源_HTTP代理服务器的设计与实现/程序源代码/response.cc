#include "response.h"



namespace msystem {


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
