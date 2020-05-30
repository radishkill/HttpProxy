#ifndef CONN_MANAGER_H
#define CONN_MANAGER_H

#include <set>
#include <memory>

#include "proxyconnection.h"

namespace msystem {

class Filter;

class ConnManager {
 public:
  ConnManager(const ConnManager&) = delete;
  ConnManager& operator=(const ConnManager&) = delete;

  /// Construct a connection manager.
  ConnManager();

  /// Add the specified connection to the manager and start it.
  void Start(std::shared_ptr<Connection> c);

  /// Stop the specified connection.
  void Stop(std::shared_ptr<Connection> c);

 private:
};

}

#endif
