#ifndef MSYSTEM_FILTER_H
#define MSYSTEM_FILTER_H
#include <string>
#include <vector>

namespace msystem {
class Filter {
 public:
  Filter(const Filter&) = delete ;
  Filter& operator=(const Filter&) = delete ;

  Filter(const std::string& filter);
  uint8_t FilterByDomain(const std::string domain);
  uint8_t FilterByUrl(const std::string url);
  ~Filter();
  static Filter* GetFilter();
  static int SetFilter(Filter* filter);
 private:
  void Parse(std::ifstream& ifs);
  std::string filter_file_name_;
  std::vector<std::string> filter_list_;
  static Filter* filter_;
};
}

#endif
