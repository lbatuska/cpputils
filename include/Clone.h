#pragma once
#include <memory>

class Clone {
public:
  virtual ~Clone() = default;
  virtual std::unique_ptr<Clone> clone() const = 0;
};
