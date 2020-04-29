#ifndef REQUEST_HANDLER_H
#define REQUEST_HANDLER_H

#include <string>

#include "request.h"
#include "conninfo.h"

namespace msystem {

class ConnHandler;

/// The common handler for all incoming requests.
class RequestHandler {
 public:
  RequestHandler(ConnHandler& conn_handler);
  RequestHandler(const RequestHandler&) = delete;
  RequestHandler& operator=(const RequestHandler&) = delete;

  int ProcessRequest();
  void ProcessClientHeaders();

  static void ComposeRequest(const Request& req, std::string& str_req);


  ///void handle_request(const request& req, reply& rep);
  ~RequestHandler();

 private:
  int ExtractUrl(const std::string& url, int default_port);
  void StripUserNameAndPassword(std::string& host);
  int StripReturnPort(std::string& host);
  void RemoveConnectionHeaders();

  ConnHandler& conn_handler_;

  /// The directory containing the files to be served.
  std::string doc_root_;
};

}

#endif
