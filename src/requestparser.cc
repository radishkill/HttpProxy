#include "requestparser.h"

namespace msystem {

RequestParser::RequestParser()
   : state_(kMethodStart) {

}

void RequestParser::Reset() {
  this->state_ = kMethodStart;
}

RequestParser::ResultType RequestParser::Consume(Request& req, char input) {
  switch (state_) {
  case kMethodStart:
    if (!IsChar(input) || IsCtl(input) || IsTspecial(input)) {
      return kBad;
    } else {
      state_ = kMethod;
      req.method.push_back(input);
      return kIndeterminate;
    }
  case kMethod:
    if (input == ' ') {
      state_ = kUri;
      return kIndeterminate;
    } else if (!IsChar(input) || IsCtl(input) || IsTspecial(input)) {
      return kBad;
    } else {
      req.method.push_back(input);
      return kIndeterminate;
    }
  case kUri:
    if (input == ' ') {
      state_ = kHttpVersionH;
      return kIndeterminate;
    } else if (IsCtl(input)) {
      return kBad;
    } else {
      req.uri.push_back(input);
      return kIndeterminate;
    }
  case kHttpVersionH:
    if (input == 'H') {
      state_ = kHttpVersionT1;
      req.http_version.push_back(input);
      return kIndeterminate;
    } else {
      return kBad;
    }
  case kHttpVersionT1:
    if (input == 'T') {
      state_ = kHttpVersionT2;
      req.http_version.push_back(input);
      return kIndeterminate;
    } else {
      return kBad;
    }
  case kHttpVersionT2:
    if (input == 'T') {
      state_ = kHttpVersionP;
      req.http_version.push_back(input);
      return kIndeterminate;
    } else {
      return kBad;
    }
  case kHttpVersionP:
    if (input == 'P') {
      state_ = kHttpVersionSlash;
      req.http_version.push_back(input);
      return kIndeterminate;
    } else {
      return kBad;
    }
  case kHttpVersionSlash:
    if (input == '/') {
      state_ = kHttpVersionMajorStart;
      req.http_version.push_back(input);
      return kIndeterminate;
    } else {
      return kBad;
    }
  case kHttpVersionMajorStart:
    if (IsDigit(input)) {
      state_ = kHttpVersionMajor;
      req.http_version.push_back(input);
      return kIndeterminate;
    } else {
      return kBad;
    }
  case kHttpVersionMajor:
    if (input == '.') {
      req.http_version.push_back(input);
      state_ = kHttpVersionMinorStart;
      return kIndeterminate;
    } else if (IsDigit(input)) {
      req.http_version.push_back(input);
      return kIndeterminate;
    } else {
      return kBad;
    }
  case kHttpVersionMinorStart:
    if (IsDigit(input)) {
      req.http_version.push_back(input);
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
      req.http_version.push_back(input);
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
    } else if (!req.headers.empty() && (input == ' ' || input == '\t')) {
      state_ = kHeaderLws;
      return kIndeterminate;
    } else if (!IsChar(input) || IsCtl(input) || IsTspecial(input)) {
      return kBad;
    } else {
      req.headers.push_back(Header());
      req.headers.back().name.push_back(input);
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
      req.headers.back().value.push_back(input);
      return kIndeterminate;
    }
  case kHeaderName:
    if (input == ':') {
      state_ = kspaceBeforeHeaderValue;
      return kIndeterminate;
    } else if (!IsChar(input) || IsCtl(input) || IsTspecial(input)) {
      return kBad;
    } else {
      req.headers.back().name.push_back(input);
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
      req.headers.back().value.push_back(input);
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

void RequestParser::SpliteToHostAndPort(const std::string& host, Request& req) {
  req.host.clear();
  std::size_t p;
  if ((p=host.find(':')) == std::string::npos) {
    if (req.method == "CONNECT") {
      req.port = HTTP_PORT_SSL;
    } else {
      req.port = HTTP_PORT;
    }
  } else {
    req.port = static_cast<uint16_t>(std::stoi(host.substr(p+1)));
    req.host = host.substr(0, p);
  }
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

