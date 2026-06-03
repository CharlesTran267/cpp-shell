#pragma once

#include <filesystem>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace fs = std::filesystem;

class PathManager {
 public:
  PathManager() {
    std::string path_env = std::getenv("PATH");
    std::stringstream ss(path_env);
    std::string path;
    while (std::getline(ss, path, ':')) m_path_dirs.push_back(path);

    for (const auto& path : m_path_dirs) {
      if (!fs::exists(path)) continue;

      for (const auto& entry : fs::directory_iterator(path)) {
        if (!entry.is_regular_file()) continue;

        auto file_name = entry.path().filename().string();
        if (m_exe2path.count(file_name)) continue;

        fs::perms perm = fs::status(entry).permissions();
        if ((perm & fs::perms::owner_exec) != fs::perms::none) {
          m_exe2path[file_name] = entry.path().string();
          m_exec.insert(file_name);
        }
      }
    }
  }

  const std::string& get_abs_path(const std::string& exec) {
    static constexpr std::string empty_string = "";

    if (m_exe2path.count(exec)) return m_exe2path[exec];
    return empty_string;
  }

  const std::unordered_set<std::string>& get_executables() { return m_exec; }

 private:
  std::vector<std::string> m_path_dirs;
  std::unordered_set<std::string> m_exec;
  std::unordered_map<std::string, std::string>
      m_exe2path;  // Mapping an executable to its absolute path
};
