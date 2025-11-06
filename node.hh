#pragma once

#include <memory>
#include <string>
class Node {
public:
  std::shared_ptr<Node> left_child = nullptr;
  std::shared_ptr<Node> right_child = nullptr;
  bool is_bottom = false;
  std::string value;
};
