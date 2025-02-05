#pragma once
#include <memory>

namespace cpputils {
class Clone {
public:
  virtual std::unique_ptr<Clone> clone() const = 0;
};
} // namespace cpputils
