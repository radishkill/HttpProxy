#ifndef REQUESTPARSER_H
#define REQUESTPARSER_H

#include <iostream>
#include <tuple>

#include "request.h"

namespace msystem {
class RequestParser {
 public:
  RequestParser();
  /// Reset to initial parser state
  void Reset();
  /// Result of parse.
  enum ResultType { kGood, kBad, kIndeterminate };
  template <typename InputIterator>
  std::tuple<ResultType, InputIterator> parse(Request& req, InputIterator begin, InputIterator end) {
    while (begin != end) {
      ResultType result = Consume(req, *begin++);
      if (result == kGood || result == kBad)
        return std::make_tuple(result, begin);
    }
    return std::make_tuple(kIndeterminate, begin);
  }
  ~RequestParser();
 private:
  /// Handle the next character of input.
  ResultType Consume(Request& req, char input);

  /// Check if a byte is an HTTP character.
  static bool IsChar(int c);

  /// Check if a byte is an HTTP control character.
  static bool IsCtl(int c);

  /// Check if a byte is defined as an HTTP tspecial character.
  static bool IsTspecial(int c);

  /// Check if a byte is a digit.
  static bool IsDigit(int c);

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
};
}

#endif
