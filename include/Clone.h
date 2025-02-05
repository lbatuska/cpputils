#pragma once
#include <memory>

class Clone {
public:
  virtual std::unique_ptr<Clone> clone() const = 0;
};
