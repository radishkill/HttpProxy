#include "filter.h"

#include <iostream>
#include <array>
#include <fstream>
#include <regex>

#define MAX_BUFFER 1024

namespace msystem {

Filter::Filter(const std::string& f)
    : filter_file_name_(f) {
  std::ifstream ifs(filter_file_name_);
  if (!ifs.is_open())
    throw std::string("can not open filter file: ") + filter_file_name_;
  Parse(ifs);
  ifs.close();
}

uint8_t Filter::FilterByDomain(const std::string domain) {
  for (auto i : filter_list_) {
    std::regex r(i + "(:\\d{1,5})?");
    if (std::regex_match(domain, r)) {
      return false;
    }
  }
  return true;
}

uint8_t Filter::FilterByUrl(const std::string url) {
  for (auto i : filter_list_) {
    std::regex r(i);
    if (std::regex_match(url, r)) {
      return false;
    }
  }
  return true;
}



Filter::~Filter() {

}

Filter* Filter::filter_ = nullptr;

Filter *Filter::GetFilter() {
  return filter_;
}

int Filter::SetFilter(Filter *filter) {
  Filter::filter_ = filter;
  return 0;
}

void Filter::Parse(std::ifstream &ifs) {
  std::array<char, MAX_BUFFER> buff;
  std::string l;
  uint8_t skip = false;
  while (!ifs.eof()) {
    std::streamsize size = ifs.readsome(buff.data(), buff.max_size());
    if (size == 0) {
      break;
    }
    for (auto iter = buff.begin(); iter != buff.begin()+size; ++iter) {
      if (l.empty() && *iter == '#') {
        skip = true;
      } else if (l.empty() && *iter == ' ') {

      } else if (*iter == '\n') {
        if (!skip)
          filter_list_.push_back(l);
        l.clear();
        skip = false;
      } else {
        l.push_back(*iter);
      }
    }
  }
}

}
