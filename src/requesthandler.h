#ifndef REQUEST_HANDLER_H
#define REQUEST_HANDLER_H

#include <string>

namespace msystem {

class Request;

/// The common handler for all incoming requests.
class RequestHandler {
 public:
  RequestHandler(const RequestHandler&) = delete;
  RequestHandler& operator=(const RequestHandler&) = delete;

  /// Construct with a directory containing files to be served.
  RequestHandler();

  /// Handle a request and produce a reply.
  void HandleRequest(const Request& cli_req, Request& ser_req);

  ///void handle_request(const request& req, reply& rep);
  ~RequestHandler();

 private:
  /// The directory containing the files to be served.
  std::string doc_root_;
};

}

#endif
