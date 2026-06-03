#pragma once

#include <array>
#include <memory>
#include <print>
#include <queue>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class Trie {
  struct Node {
    Node() = default;
    Node(const std::string& s) : s(std::move(s)) {}

    std::string s;
    bool is_leaf = false;
    std::array<std::unique_ptr<Node>, 128> children;
  };

 public:
  Trie() : root(new Node()) {}

  void insert(const std::string& s) {
    Node* cur = root.get();
    for (int i = 0; char c : s) {
      if (!cur->children[c]) {
        cur->children[c] = std::make_unique<Node>(s.substr(0, i + 1));
      }
      cur = cur->children[c].get();
      i++;
    }
    cur->is_leaf = true;
  }

  std::vector<std::string> query(const std::string& pre) {
    Node* cur = root.get();
    for (char c : pre) {
      if (!cur->children[c]) return {};
      cur = cur->children[c].get();
    }

    std::vector<std::string> ans;
    std::queue<Node*> q;
    q.push(cur);

    while (!q.empty()) {
      Node* f = q.front();
      q.pop();

      for (const auto& child : f->children) {
        if (child) {
          q.push(child.get());
          if (child->is_leaf) ans.emplace_back(child->s);
        }
      }
    }
    return ans;
  }

 private:
  std::unique_ptr<Node> root;
};

class AutoCompleter {
 public:
  AutoCompleter() = default;
  AutoCompleter(const std::vector<std::string>& words) {
    for (const auto& w : words) m_trie.insert(w);
  }

  std::vector<std::string> query(const std::string& pre) {
    if (cache_query.count(pre)) return cache_query.at(pre);
    return cache_query[pre] = m_trie.query(pre);
  }

  void insert(const std::string& word) { m_trie.insert(word); }
  void insert(const std::vector<std::string>& words) {
    for (const auto& w : words) m_trie.insert(w);
  }
  void insert(const std::unordered_set<std::string>& words) {
    for (const auto& w : words) m_trie.insert(w);
  }

 private:
  std::unordered_map<std::string, std::vector<std::string>> cache_query;
  Trie m_trie;
};
