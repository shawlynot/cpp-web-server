//
// Created by shawlynot on 2/4/24.
//

#include <vector>
#include <string>
#include <optional>
#include <algorithm>
#include "util.h"

namespace shawlynot {

  /**
   * Split a string by a separator into a vector
   */
  std::vector<std::string> split_string(const std::string &request_str, const std::string &separator) {
    std::vector<std::string> out;
    unsigned long start_pos = 0;
    unsigned long end_pos;
    for (; start_pos < request_str.length(); start_pos = end_pos + separator.length()) {
      end_pos = request_str.find(separator, start_pos);
      if (end_pos == std::string::npos) {
        end_pos = request_str.length();
      }
      out.push_back(request_str.substr(start_pos, end_pos - start_pos));
    }
    return out;
  }

} // shawlynot