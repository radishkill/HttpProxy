#include "connmanager.h"


namespace msystem {

ConnManager::ConnManager() {
}

void ConnManager::Start(std::shared_ptr<Connection> c) {
  c->Run();
}

void ConnManager::Stop(std::shared_ptr<Connection> c) {
  c->Shutdown();
}


}
