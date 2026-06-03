#pragma once

#include <cstddef>
#include <cstdlib>
#include <fstream>
#include <print>
#include <sstream>
#include <string>
#include <vector>

class HistoryManager {
 public:
  HistoryManager() {
    m_hist_file = std::getenv("HISTFILE");
    if (m_hist_file) {
      read_hist(m_hist_file);
      m_last_append = m_cmd_history.size() - 1;
    }
  }

  ~HistoryManager() {
    if (m_hist_file) append_hist(m_hist_file);
  }

  void print_hist(int n) {
    for (int i = std::max(0, int(m_cmd_history.size()) - n);
         i < m_cmd_history.size(); i++) {
      std::println("    {}  {}", i + 1, m_cmd_history[i]);
    }
  }

  void read_hist(const std::string& path) {
    std::fstream ifs(path, std::fstream::in);
    std::string hist_cmd;
    while (std::getline(ifs, hist_cmd)) m_cmd_history.push_back(hist_cmd);
  }

  void write_hist(const std::string& path) {
    std::fstream ofs(path, std::fstream::out);
    for (const auto& cmd : m_cmd_history) ofs << cmd << '\n';
  }

  void append_hist(const std::string& path) {
    std::fstream ofs(path, std::fstream::app);
    for (int i = m_last_append + 1; i < m_cmd_history.size(); i++)
      ofs << m_cmd_history[i] << '\n';

    m_last_append = m_cmd_history.size() - 1;
  }

  void handle_hist(const std::string& cmd_input) {
    if (cmd_input.empty()) {
      print_hist(m_cmd_history.size());
      return;
    }

    std::vector<std::string> args;
    std::stringstream ss(cmd_input);
    std::string arg;
    while (std::getline(ss, arg, ' ')) args.push_back(arg);

    if (args.size() == 1) {
      print_hist(std::stoi(args[0]));
    } else if (args.size() == 2) {
      if (args[0] == "-r")
        read_hist(args[1]);
      else if (args[0] == "-w")
        write_hist(args[1]);
      else if (args[0] == "-a")
        append_hist(args[1]);
    }
  }

  void push(const std::string& cmd) { m_cmd_history.push_back(cmd); }
  const std::string& get(std::size_t idx) { return m_cmd_history[idx]; }
  std::size_t size() { return m_cmd_history.size(); }
  bool empty() { return m_cmd_history.empty(); }

  const std::string& front() { return m_cmd_history.front(); }
  const std::string& back() { return m_cmd_history.back(); }

  const std::string& get_next() {
    static constexpr std::string empty_str = "";

    if (m_cur_idx < m_cmd_history.size()) m_cur_idx++;
    if (!m_cmd_history.empty() && m_cur_idx < m_cmd_history.size())
      return m_cmd_history[m_cur_idx];
    else
      return empty_str;
  }

  const std::string& get_prev() {
    static constexpr std::string empty_str = "";

    if (m_cur_idx > 0) m_cur_idx--;
    if (!m_cmd_history.empty())
      return m_cmd_history[m_cur_idx];
    else
      return empty_str;
  }
  void reset_idx() { m_cur_idx = m_cmd_history.size(); }

 private:
  std::size_t m_cur_idx;
  const char* m_hist_file = NULL;
  int m_last_append = -1;
  std::vector<std::string> m_cmd_history;
};
