#ifndef MSYSTEM_UTILS_H
#define MSYSTEM_UTILS_H
#include <tuple>
#include <string>

namespace msystem {
class Utils {
 public:
  static std::tuple<std::string, int> SpliteHost(const std::string& host);
};
}

#endif // UTILS_H
