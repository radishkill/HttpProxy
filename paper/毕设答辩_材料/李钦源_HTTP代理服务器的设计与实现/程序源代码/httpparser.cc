#include "httpparser.h"

namespace msystem {

HttpParser::HttpParser(HttpProtocol* http)
    : state_(kProtocolStart),
      http_(http) {
}


HttpParser::ResultType HttpParser::ConsumeRequest(char input) {
  switch (state_) {
  case kProtocolStart:
    if (!IsChar(input) || IsCtl(input) || IsTspecial(input)) {
      return kBad;
    } else {
      state_ = kMethod;
      http_->method.push_back(input);
      return kIndeterminate;
    }
  case kMethod:
    if (input == ' ') {
      state_ = kUri;
      return kIndeterminate;
    } else if (!IsChar(input) || IsCtl(input) || IsTspecial(input)) {
      return kBad;
    } else {
      http_->method.push_back(input);
      return kIndeterminate;
    }
  case kUri:
    if (input == ' ') {
      state_ = kHttpVersionH;
      return kIndeterminate;
    } else if (IsCtl(input)) {
      return kBad;
    } else {
      http_->raw_url.push_back(input);
      return kIndeterminate;
    }
  case kHttpVersionH:
    if (input == 'H') {
      state_ = kHttpVersionT1;
      http_->http_version.push_back(input);
      return kIndeterminate;
    } else {
      return kBad;
    }
  case kHttpVersionT1:
    if (input == 'T') {
      state_ = kHttpVersionT2;
      http_->http_version.push_back(input);
      return kIndeterminate;
    } else {
      return kBad;
    }
  case kHttpVersionT2:
    if (input == 'T') {
      state_ = kHttpVersionP;
      http_->http_version.push_back(input);
      return kIndeterminate;
    } else {
      return kBad;
    }
  case kHttpVersionP:
    if (input == 'P') {
      state_ = kHttpVersionSlash;
      http_->http_version.push_back(input);
      return kIndeterminate;
    } else {
      return kBad;
    }
  case kHttpVersionSlash:
    if (input == '/') {
      state_ = kHttpVersionMajorStart;
      http_->http_version.push_back(input);
      return kIndeterminate;
    } else {
      return kBad;
    }
  case kHttpVersionMajorStart:
    if (IsDigit(input)) {
      state_ = kHttpVersionMajor;
      http_->http_version.push_back(input);
      return kIndeterminate;
    } else {
      return kBad;
    }
  case kHttpVersionMajor:
    if (input == '.') {
      http_->http_version.push_back(input);
      state_ = kHttpVersionMinorStart;
      return kIndeterminate;
    } else if (IsDigit(input)) {
      http_->http_version.push_back(input);
      return kIndeterminate;
    } else {
      return kBad;
    }
  case kHttpVersionMinorStart:
    if (IsDigit(input)) {
      http_->http_version.push_back(input);
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
      http_->http_version.push_back(input);
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
    } else if (!name.empty() && (input == ' ' || input == '\t')) {
      state_ = kHeaderLws;
      return kIndeterminate;
    } else if (!IsChar(input) || IsCtl(input) || IsTspecial(input)) {
      return kBad;
    } else {
      name.clear();
      value.clear();
      if (IsUpper(input))
        input += 32;
      name.push_back(input);
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
      value.push_back(input);
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
      name.push_back(input);
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
      value.push_back(input);
      return kIndeterminate;
    }
  case kExpectingNewline2:
    http_->headers.insert(std::make_pair(name, value));
    if (input == '\n') {
      state_ = kHeaderLineStart;
      return kIndeterminate;
    } else {
      return kBad;
    }
  case kExpectingNewline3:
    if (input == '\n') {
      auto iter = http_->headers.find("content-length");
      if (iter != http_->headers.end()) {
        try {
        http_->data_length = boost::lexical_cast<std::size_t>(iter->second);
        } catch (...) {
          return kBad;
        }
        state_ = kHttpData;
        return kIndeterminate;
      } else {
        http_->data_length = 0;
        return kGood;
      }
    } else {
      return kBad;
    }
  case kHttpData:
    http_->data.push_back(input);
    if (http_->data.length() >= http_->data_length)
      return kGood;
    else
      return kIndeterminate;
  default:
    return kBad;
  }
}

HttpParser::ResultType HttpParser::ConsumeResponse(char input) {
  switch (state_) {
  case kProtocolStart: {
    if (!IsChar(input) || IsCtl(input) || IsTspecial(input)) {
      return kBad;
    } else {
      state_ = kHttpVersionT1;
      http_->http_version.push_back(input);
      return kIndeterminate;
    }
  }
  case kHttpVersionH:
    if (input == 'H') {
      state_ = kHttpVersionT1;
      http_->http_version.push_back(input);
      return kIndeterminate;
    } else {
      return kBad;
    }
  case kHttpVersionT1:
    if (input == 'T') {
      state_ = kHttpVersionT2;
      http_->http_version.push_back(input);
      return kIndeterminate;
    } else {
      return kBad;
    }
  case kHttpVersionT2:
    if (input == 'T') {
      state_ = kHttpVersionP;
      http_->http_version.push_back(input);
      return kIndeterminate;
    } else {
      return kBad;
    }
  case kHttpVersionP:
    if (input == 'P') {
      state_ = kHttpVersionSlash;
      http_->http_version.push_back(input);
      return kIndeterminate;
    } else {
      return kBad;
    }
  case kHttpVersionSlash:
    if (input == '/') {
      state_ = kHttpVersionMajorStart;
      http_->http_version.push_back(input);
      return kIndeterminate;
    } else {
      return kBad;
    }
  case kHttpVersionMajorStart:
    if (IsDigit(input)) {
      state_ = kHttpVersionMajor;
      http_->http_version.push_back(input);
      return kIndeterminate;
    } else {
      return kBad;
    }
  case kHttpVersionMajor:
    if (input == '.') {
      http_->http_version.push_back(input);
      state_ = kHttpVersionMinorStart;
      return kIndeterminate;
    } else if (IsDigit(input)) {
      http_->http_version.push_back(input);
      return kIndeterminate;
    } else {
      return kBad;
    }
  case kHttpVersionMinorStart:
    if (IsDigit(input)) {
      http_->http_version.push_back(input);
      state_ = kHttpVersionMinor;
      return kIndeterminate;
    } else {
      return kBad;
    }
  case kHttpVersionMinor:
    if (input == ' ') {
      state_ = kHttpStatusCode;
      return kIndeterminate;
    } else if (IsDigit(input)) {
      http_->http_version.push_back(input);
      return kIndeterminate;
    } else {
      return kBad;
    }
  case kHttpStatusCode: {
    if (input == ' ') {
      state_ = kHttpReasonPhrase;
      return kIndeterminate;
    } else if (IsDigit(input)) {
      http_->status_code.push_back(input);
      return kIndeterminate;
    } else {
      return kBad;
    }
  }
  case kHttpReasonPhrase: {
    if (input == '\r') {
      state_ = kexpectingNewline1;
      return kIndeterminate;
    } else if (IsChar(input)) {
      http_->reason_phrase.push_back(input);
      return kIndeterminate;
    } else {
      return kBad;
    }
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
    } else if (!name.empty() && (input == ' ' || input == '\t')) {
      state_ = kHeaderLws;
      return kIndeterminate;
    } else if (!IsChar(input) || IsCtl(input) || IsTspecial(input)) {
      return kBad;
    } else {
      name.clear();
      value.clear();
      if (IsUpper(input))
        input += 32;
      name.push_back(input);
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
      value.push_back(input);
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
      name.push_back(input);
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
      value.push_back(input);
      return kIndeterminate;
    }
  case kExpectingNewline2:
    http_->headers.insert(std::make_pair(name, value));
    if (input == '\n') {
      state_ = kHeaderLineStart;
      return kIndeterminate;
    } else {
      return kBad;
    }
  case kExpectingNewline3:
    if (input == '\n') {
      auto iter = http_->headers.find("content-length");
      if (iter != http_->headers.end()) {
        http_->data_length = boost::lexical_cast<std::size_t>(iter->second);
        state_ = kHttpData;
        return kIndeterminate;
      } else {
        http_->data_length = 0;
        return kGood;
      }
    } else {
      return kBad;
    }
  case kHttpData:
    http_->data.push_back(input);
    if (http_->data.length() >= http_->data_length)
      return kGood;
    else
      return kIndeterminate;
  default:
    return kBad;
  }
}


bool HttpParser::IsUpper(int c) {
  return c >= 'A' && c <= 'Z';
}



bool HttpParser::IsChar(int c) {
  return c >= 0 && c <= 127;
}

bool HttpParser::IsCtl(int c) {
  return (c >= 0 && c <= 31) || (c == 127);
}

bool HttpParser::IsTspecial(int c)
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

bool HttpParser::IsDigit(int c)
{
  return c >= '0' && c <= '9';
}

HttpParser::~HttpParser() {

}

}

