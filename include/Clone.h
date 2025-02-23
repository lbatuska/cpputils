#pragma once
#include <memory>

namespace cpputils {
template <typename T>
class Clone {
 public:
  virtual std::unique_ptr<T> clone() const noexcept = 0;
};
}  // namespace cpputils
