#include "connmanager.h"

namespace msystem {

ConnManager::ConnManager() {

}

void ConnManager::Start(std::shared_ptr<ConnHandler> c) {
  connections_.insert(c);
  c->Run();
}

void ConnManager::Stop(std::shared_ptr<ConnHandler> c) {
  connections_.erase(c);
  c->Stop();
}

void ConnManager::StopAll() {
  for (auto c : connections_)
    c->Stop();
  connections_.clear();
}

}
