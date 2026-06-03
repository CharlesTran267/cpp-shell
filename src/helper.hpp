#pragma once

#include <print>
#include <sstream>
#include <string>
#include <vector>

inline void strip(std::string& str, char c = ' ') {
  int i = 0;
  for (; i < str.size(); i++) {
    if (str[i] != c) break;
  }

  int j = str.size() - 1;
  for (; j >= 0; j--) {
    if (str[j] != c) break;
  }

  str = str.substr(i, j - i + 1);
}

inline std::vector<std::string> split(std::string str, char c = ' ') {
  std::vector<std::string> ans;

  std::stringstream ss(str);
  std::string temp;
  while (std::getline(ss, temp, c)) {
    ans.push_back(temp);
  }

  return ans;
}
