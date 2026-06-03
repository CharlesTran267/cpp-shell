#include <sys/wait.h>
#include <term.h>
#include <unistd.h>

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <print>
#include <string>
#include <unordered_set>
#include <vector>

#include "auto_completion.hpp"
#include "helper.hpp"
#include "history_manager.hpp"
#include "path_manager.hpp"

namespace fs = std::filesystem;

constexpr auto UP_ARROW = "\033[A";
constexpr auto DOWN_ARROW = "\033[B";
constexpr auto RIGHT_ARROW = "\033[C";
constexpr auto LEFT_ARROW = "\033[D";

constexpr auto EXIT_CMD = "exit";
constexpr auto ECHO_CMD = "echo";
constexpr auto TYPE_CMD = "type";
constexpr auto PWD_CMD = "pwd";
constexpr auto CD_CMD = "cd";
constexpr auto HIST_CMD = "history";
static const std::unordered_set<std::string> builtin_cmd_set = {
    EXIT_CMD, ECHO_CMD, TYPE_CMD, PWD_CMD, CD_CMD, HIST_CMD};

static auto hist_manager = HistoryManager();
static auto path_manager = PathManager();
static auto auto_completer = AutoCompleter();

void handle_type(const std::string& input) {
  auto it = builtin_cmd_set.find(input);
  if (it != builtin_cmd_set.end())
    std::println("{} is a shell builtin", input);
  else {
    std::string full_path = path_manager.get_abs_path(input);
    if (full_path.empty())
      std::println("{}: not found", input);
    else
      std::println("{} is {}", input, full_path);
  }
}

void handle_echo(const std::string& input) { std::println("{}", input); }

void handle_pwd() { std::println("{}", fs::current_path().string()); }

void handle_cd(const std::string& path) {
  const static auto home_path = std::getenv("HOME");
  try {
    if (path == "~")
      fs::current_path(home_path);
    else
      fs::current_path(path);
  } catch (const fs::filesystem_error& e) {
    std::println("cd: {}: No such file or directory", path);
  }
}

void handle_hist(const std::string& cmd_input) {
  // Usage:
  // 1. history: print all history commands
  // 2. history <n>: print last n commands (including the history command)
  // 3. history -r/w/a: read/write/append to a history file
  hist_manager.handle_hist(cmd_input);
}

void eval(const std::string& input) {
  auto it = std::find(input.begin(), input.end(), ' ');
  std::string cmd(input.begin(), it);
  std::string cmd_input;
  if (it != input.end()) {
    cmd_input = std::string(it + 1, input.end());
  }

  if (cmd == ECHO_CMD)
    handle_echo(cmd_input);
  else if (cmd == TYPE_CMD)
    handle_type(cmd_input);
  else if (cmd == PWD_CMD)
    handle_pwd();
  else if (cmd == CD_CMD)
    handle_cd(cmd_input);
  else if (cmd == HIST_CMD)
    handle_hist(cmd_input);
  else {
    std::string full_path = path_manager.get_abs_path(cmd);
    if (full_path.empty()) {
      std::println("{}: command not found", cmd);
      return;
    }

    std::vector<std::string> args = split(cmd_input);
    std::vector<char*> argv;
    argv.push_back(cmd.data());
    for (const auto& arg : args) argv.push_back(const_cast<char*>(arg.data()));
    argv.push_back(nullptr);

    int pid = fork();
    if (!pid) {
      execvp(full_path.c_str(), argv.data());
    } else {
      int status;
      waitpid(pid, &status, 0);
    }
  }
}

void process(const std::string& input) {
  auto cmds = split(input, '|');

  if (cmds.size() == 1) {
    strip(cmds[0]);
    eval(cmds[0]);
    return;
  }

  for (int i = 0; i < cmds.size() - 1; i++) {
    strip(cmds[i]);
    strip(cmds[i + 1]);

    int fd[2];
    pipe(fd);

    int pid1 = fork();
    if (!pid1) {
      // child: Forward from stdout to fd[1];
      close(fd[0]);  // avoid read
      dup2(fd[1], STDOUT_FILENO);
      eval(cmds[i]);
      close(fd[1]);
      return;
    }

    int pid2 = fork();
    if (!pid2) {
      // parent: Forward from fd[0] to stdin;
      close(fd[1]);  // avoid read
      dup2(fd[0], STDIN_FILENO);

      int status;
      waitpid(pid1, &status, 0);
      eval(cmds[i + 1]);
    } else {
      int status;
      waitpid(pid2, &status, 0);
    }
  }
}

void enable_raw_mode() {
  termios term;
  tcgetattr(STDIN_FILENO, &term);
  term.c_lflag &= ~(ICANON | ECHO);  // disable line buffering and echo
  tcsetattr(STDIN_FILENO, TCSANOW, &term);
}

std::string longest_common_substr(const std::vector<std::string>& candidates,
                                  int start_idx) {
  if (candidates.empty()) return "";
  if (candidates.size() == 1) return candidates[0];

  while (true) {
    if (candidates[0].size() <= start_idx) return candidates[0];

    char c = candidates[0][start_idx];
    for (const auto& can : candidates) {
      if (can.size() <= start_idx) {
        return can;
      }
      if (can[start_idx] != c) return can.substr(0, start_idx);
    }
    start_idx++;
  }
  return "";
}

char handle_tab(std::string& cmd_pref) {
  auto candidates = auto_completer.query(cmd_pref);
  int consec = 1;

  char c = '\t';
  std::string temp;
  while (c == '\t') {
    if (candidates.empty()) {
      // Ring the bell to signal not found
      std::cout << '\x07';
    } else if (candidates.size() == 1) {
      // If there is only one candidate, auto complete.
      std::string left(candidates[0].begin() + cmd_pref.size(),
                       candidates[0].end());
      std::cout << left << ' ';
      cmd_pref = candidates[0] + ' ';
      candidates.clear();
    } else {
      // If there are multiple candidates, try to do partial complete first
      temp = longest_common_substr(candidates, cmd_pref.size());
      if (temp.size() > cmd_pref.size()) {
        cmd_pref = temp;
        if (candidates.size() == 1) cmd_pref += ' ';
        std::cout << "\r\033[K";  // replace current line
        std::print("$ {}", cmd_pref);

        if (candidates.size() == 1)
          candidates.clear();
        else
          candidates = auto_completer.query(cmd_pref);

        consec = 0;
      } else {
        if (consec == 1 || candidates.empty())
          std::cout << '\x07';
        else {
          if (consec > 2) {
            cmd_pref = candidates[(consec - 3) % candidates.size()] + ' ';
            std::cout << "\r\033[K";  // replace current line
          } else {
            std::cout << '\n';
            for (int i = 0; i < candidates.size(); i++) {
              if (i < candidates.size() - 1)
                std::print("{}  ", candidates[i]);
              else
                std::println("{}", candidates[i]);
            }
          }
          std::print("$ {}", cmd_pref);
        }
      }
    }

    std::cin.get(c);
    consec++;
  }
  return c;
}

int main() {
  // Flush after every std::cout / std:cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  enable_raw_mode();

  auto_completer.insert(builtin_cmd_set);
  const auto& exec = path_manager.get_executables();
  auto_completer.insert(exec);

  while (true) {
    hist_manager.reset_idx();

    std::cout << "$ ";
    std::string cmd;

    char c;
    while (std::cin.get(c)) {
      if (c == '\t') c = handle_tab(cmd);

      if (c == '\n') {
        std::cout << c;
        break;
      } else if (c == '\033') {
        char seq[2];
        std::cin.get(seq[0]);
        std::cin.get(seq[1]);

        if (seq[0] == '[' && (seq[1] == 'A' || seq[1] == 'B')) {
          std::cout << "\r\033[K";  // replace current line
          switch (seq[1]) {
            case 'A': {
              cmd = hist_manager.get_prev();
              std::print("$ {}", cmd);
              break;
            }
            case 'B': {
              cmd = hist_manager.get_next();
              std::print("$ {}", cmd);
              break;
            }
            default:
              break;
          }
        } else
          std::cout << c;
      } else if (c == 127) {
        if (!cmd.empty()) {
          cmd.pop_back();
          std::cout << "\b \b";
        }
      } else {
        cmd += c;
        std::cout << c;
      }
    }

    if (hist_manager.empty() || cmd != hist_manager.back())
      hist_manager.push(cmd);
    if (!cmd.find(EXIT_CMD)) break;
    process(cmd);
  }
}
