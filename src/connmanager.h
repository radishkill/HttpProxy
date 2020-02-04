#ifndef CONN_MANAGER_H
#define CONN_MANAGER_H

#include <set>

#include "connhandler.h"

namespace msystem {

class ConnManager {
 public:
  ConnManager(const ConnManager&) = delete;
  ConnManager& operator=(const ConnManager&) = delete;

  /// Construct a connection manager.
  ConnManager();

  /// Add the specified connection to the manager and start it.
  void Start(std::shared_ptr<ConnHandler> c);

  /// Stop the specified connection.
  void Stop(std::shared_ptr<ConnHandler> c);

  /// Stop all connections.
  void StopAll();
 private:
  /// The managed connections.
  std::set<std::shared_ptr<ConnHandler>> connections_;
};

}

#endif
