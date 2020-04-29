#ifndef REQUESTPARSER_H
#define REQUESTPARSER_H

#include <iostream>
#include <tuple>
#include <algorithm>


#include "request.h"
#include "utils.h"

namespace msystem {
class RequestParser {
 public:
  RequestParser(Request& req);
  /// Reset to initial parser state
  void Reset();
  /// Result of parse.
  enum ResultType { kGood, kBad, kIndeterminate };
  template <typename InputIterator>
  std::tuple<ResultType, InputIterator> Parse(InputIterator begin, InputIterator end) {
    while (begin != end) {
      ResultType result = Consume(*begin++);
      if (result == kGood || result == kBad)
        return std::make_tuple(result, begin);
    }
    return std::make_tuple(kIndeterminate, begin);
  }
  ~RequestParser();
 private:
  /// Handle the next character of input.
  ResultType Consume(char input);



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
    kExpectingNewline3
  } state_;
  Request& req_;
};
}

#endif
