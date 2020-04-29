#include "conf.h"

namespace msystem {

ConfigPool* ConfigPool::config_pool_ = nullptr;

ConfigPool::ConfigPool()
    : port(0),
      idletimeout(5) {

}

ConfigPool *ConfigPool::GetConfigPool() {
  return ConfigPool::config_pool_;
}

int ConfigPool::SetConfigPool(ConfigPool *config_pool) {
  ConfigPool::config_pool_ = config_pool;
  return 0;
}

ConfigPool::~ConfigPool() {

}


}
