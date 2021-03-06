#ifndef REQUEST_HANDLER_H
#define REQUEST_HANDLER_H

#include <string>
#include "httpparser.h"

namespace msystem {

class Connection;

/// The common handler for all incoming requests.
class RequestHandler {
 public:
  RequestHandler(Connection& conn_handler);
  RequestHandler(const RequestHandler&) = delete;
  RequestHandler& operator=(const RequestHandler&) = delete;

  int ProcessRequest();
  void ProcessClientHeaders();


  ///void handle_request(const request& req, reply& rep);
  ~RequestHandler();

 private:
//  int ExtractUrl(const std::string& url, int default_port);
//  void StripUserNameAndPassword(std::string& host);
//  int StripReturnPort(std::string& host);
//  void RemoveConnectionHeaders();

  Connection& conn_;

  /// The directory containing the files to be served.
  std::string doc_root_;
};

}

#endif
