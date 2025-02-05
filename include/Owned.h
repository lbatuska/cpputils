#pragma once

namespace cpputils {
class Owned {
protected:
  Owned() {}
  ~Owned() {}
  Owned(const Owned &) = delete;
  Owned &operator=(const Owned &) = delete;
  // Support moves
  Owned(Owned &&) noexcept(true) = default;
  Owned &operator=(Owned &&) noexcept(true) = default;
};
} // namespace cpputils
