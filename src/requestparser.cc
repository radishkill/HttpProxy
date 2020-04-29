#include "requestparser.h"

namespace msystem {

RequestParser::RequestParser(Request &req)
    : state_(kMethodStart),
      req_(req) {

}

void RequestParser::Reset() {
  this->state_ = kMethodStart;
}

RequestParser::ResultType RequestParser::Consume(char input) {
  switch (state_) {
  case kMethodStart:
    if (!IsChar(input) || IsCtl(input) || IsTspecial(input)) {
      return kBad;
    } else {
      state_ = kMethod;
      req_.method.push_back(input);
      return kIndeterminate;
    }
  case kMethod:
    if (input == ' ') {
      state_ = kUri;
      return kIndeterminate;
    } else if (!IsChar(input) || IsCtl(input) || IsTspecial(input)) {
      return kBad;
    } else {
      req_.method.push_back(input);
      return kIndeterminate;
    }
  case kUri:
    if (input == ' ') {
      state_ = kHttpVersionH;
      return kIndeterminate;
    } else if (IsCtl(input)) {
      return kBad;
    } else {
      req_.url.push_back(input);
      return kIndeterminate;
    }
  case kHttpVersionH:
    if (input == 'H') {
      state_ = kHttpVersionT1;
      req_.http_version.push_back(input);
      return kIndeterminate;
    } else {
      return kBad;
    }
  case kHttpVersionT1:
    if (input == 'T') {
      state_ = kHttpVersionT2;
      req_.http_version.push_back(input);
      return kIndeterminate;
    } else {
      return kBad;
    }
  case kHttpVersionT2:
    if (input == 'T') {
      state_ = kHttpVersionP;
      req_.http_version.push_back(input);
      return kIndeterminate;
    } else {
      return kBad;
    }
  case kHttpVersionP:
    if (input == 'P') {
      state_ = kHttpVersionSlash;
      req_.http_version.push_back(input);
      return kIndeterminate;
    } else {
      return kBad;
    }
  case kHttpVersionSlash:
    if (input == '/') {
      state_ = kHttpVersionMajorStart;
      req_.http_version.push_back(input);
      return kIndeterminate;
    } else {
      return kBad;
    }
  case kHttpVersionMajorStart:
    if (IsDigit(input)) {
      state_ = kHttpVersionMajor;
      req_.http_version.push_back(input);
      return kIndeterminate;
    } else {
      return kBad;
    }
  case kHttpVersionMajor:
    if (input == '.') {
      req_.http_version.push_back(input);
      state_ = kHttpVersionMinorStart;
      return kIndeterminate;
    } else if (IsDigit(input)) {
      req_.http_version.push_back(input);
      return kIndeterminate;
    } else {
      return kBad;
    }
  case kHttpVersionMinorStart:
    if (IsDigit(input)) {
      req_.http_version.push_back(input);
      state_ = kHttpVersionMinor;
      return kIndeterminate;
    } else {
      return kBad;
    }
  case kHttpVersionMinor:
    if (input == '\r') {
      state_ = kexpectingNewline1;
      return kIndeterminate;
    } else if (IsDigit(input)) {
      req_.http_version.push_back(input);
      return kIndeterminate;
    } else {
      return kBad;
    }
  case kexpectingNewline1:
    if (input == '\n') {
      state_ = kHeaderLineStart;
      return kIndeterminate;
    } else {
      return kBad;
    }
  case kHeaderLineStart:
    if (input == '\r') {
      state_ = kExpectingNewline3;
      return kIndeterminate;
    } else if (!req_.headers.empty() && (input == ' ' || input == '\t')) {
      state_ = kHeaderLws;
      return kIndeterminate;
    } else if (!IsChar(input) || IsCtl(input) || IsTspecial(input)) {
      return kBad;
    } else {
      req_.headers.push_back(Header());
      if (IsUpper(input))
        input += 32;
      req_.headers.back().name.push_back(input);
      state_ = kHeaderName;
      return kIndeterminate;
    }
  case kHeaderLws:
    if (input == '\r') {
      state_ = kExpectingNewline2;
      return kIndeterminate;
    } else if (input == ' ' || input == '\t') {
      return kIndeterminate;
    } else if (IsCtl(input)) {
      return kBad;
    } else {
      state_ = kHeaderValue;
      req_.headers.back().value.push_back(input);
      return kIndeterminate;
    }
  case kHeaderName:
    if (input == ':') {
      state_ = kspaceBeforeHeaderValue;
      return kIndeterminate;
    } else if (!IsChar(input) || IsCtl(input) || IsTspecial(input)) {
      return kBad;
    } else {
      if (IsUpper(input))
        input += 32;
      req_.headers.back().name.push_back(input);
      return kIndeterminate;
    }
  case kspaceBeforeHeaderValue:
    if (input == ' ') {
      state_ = kHeaderValue;
      return kIndeterminate;
    } else {
      return kBad;
    }
  case kHeaderValue:
    if (input == '\r') {
      state_ = kExpectingNewline2;
      return kIndeterminate;
    } else if (IsCtl(input)) {
      return kBad;
    } else {
      req_.headers.back().value.push_back(input);
      return kIndeterminate;
    }
  case kExpectingNewline2:
    if (input == '\n') {
      state_ = kHeaderLineStart;
      return kIndeterminate;
    } else {
      return kBad;
    }
  case kExpectingNewline3:
    return (input == '\n') ? kGood : kBad;
  default:
    return kBad;
  }
}


bool RequestParser::IsUpper(int c) {
  return c >= 'A' && c <= 'Z';
}



bool RequestParser::IsChar(int c) {
  return c >= 0 && c <= 127;
}

bool RequestParser::IsCtl(int c) {
  return (c >= 0 && c <= 31) || (c == 127);
}

bool RequestParser::IsTspecial(int c)
{
  switch (c) {
  case '(': case ')': case '<': case '>': case '@':
  case ',': case ';': case ':': case '\\': case '"':
  case '/': case '[': case ']': case '?': case '=':
  case '{': case '}': case ' ': case '\t':
    return true;
  default:
    return false;
  }
}

bool RequestParser::IsDigit(int c)
{
  return c >= '0' && c <= '9';
}

RequestParser::~RequestParser() {

}

}

