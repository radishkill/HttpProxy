#ifndef MSYSTEM_RESPONSE_H
#define MSYSTEM_RESPONSE_H

#include <string>

namespace msystem {
class Response {
 public:
  static void GetSslResponse(std::string& str_response);
  static void ExpectationFailed(std::string& str_response);
  static void BadRequest(std::string& str_response);
 private:
};
}

#endif
