#ifndef MSYSTEM_HTTP_PARSER_H
#define MSYSTEM_HTTP_PARSER_H

#include <iostream>
#include <tuple>
#include <algorithm>
#include <string>
#include <vector>

#include <boost/unordered_map.hpp>
#include <boost/lexical_cast.hpp>

#include "utils.h"

namespace msystem {

#define HTTP_PORT 80
#define HTTP_PORT_SSL 443

typedef boost::unordered_map<std::string,std::string> HeadersMap;

struct HttpProtocol {
  std::string method;
  std::string raw_url;
  std::string url;
  std::string http_version;
  std::string host;
  int port;
  uint8_t protocol_major;
  uint8_t protocol_minor;
  uint8_t connect_method;
  HeadersMap headers;
  std::size_t data_length;
  std::string data;
};

class HttpParser {
 public:
  HttpParser(HttpProtocol* http);
  /// Reset to initial parser state
  void Reset(HttpProtocol* http) {
    http_ = http;
    this->state_ = kMethodStart;
  }
  /// Result of parse.
  enum ResultType { kGood, kBad, kIndeterminate };
  template <typename InputIterator>
  std::tuple<ResultType, InputIterator> ParseRequest(InputIterator begin, InputIterator end) {
    data_flag = 0;
    while (begin != end) {
      ResultType result = ConsumeRequest(*begin++);
      if (result == kGood || result == kBad)
        return std::make_tuple(result, begin);
    }
    return std::make_tuple(kIndeterminate, begin);
  }
  template <typename InputIterator>
  std::tuple<ResultType, InputIterator> ParseResponse(InputIterator begin, InputIterator end) {
    data_flag = 1;
    while (begin != end) {
      ResultType result = ConsumeResponse(*begin++);
      if (result == kGood || result == kBad)
        return std::make_tuple(result, begin);
    }
    return std::make_tuple(kIndeterminate, begin);
  }
  ~HttpParser();
 private:
  /// Handle the next character of input.
  ResultType ConsumeRequest(char input);
  ResultType ConsumeResponse(char input);


  bool IsUpper(int c);

  /// Check if a byte is an HTTP character.
   bool IsChar(int c);

  /// Check if a byte is an HTTP control character.
   bool IsCtl(int c);

  /// Check if a byte is defined as an HTTP tspecial character.
   bool IsTspecial(int c);

  /// Check if a byte is a digit.
   bool IsDigit(int c);

  /// The current state of the parser.
  enum State {
    kMethodStart,
    kMethod,
    kUri,
    kHttpVersionH,
    kHttpVersionT1,
    kHttpVersionT2,
    kHttpVersionP,
    kHttpVersionSlash,
    kHttpVersionMajorStart,
    kHttpVersionMajor,
    kHttpVersionMinorStart,
    kHttpVersionMinor,
    kexpectingNewline1,
    kHeaderLineStart,
    kHeaderLws,
    kHeaderName,
    kspaceBeforeHeaderValue,
    kHeaderValue,
    kExpectingNewline2,
    kExpectingNewline3,
    kHttpData
  } state_;
  uint8_t data_flag;
  std::string name, value;
  HttpProtocol* http_;
};
}

#endif
