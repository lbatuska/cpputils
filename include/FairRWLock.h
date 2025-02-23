#pragma once

#include <condition_variable>
#include <mutex>
#include <shared_mutex>

#include "Owned.h"

namespace cpputils {
class FairRWLock : public Owned {
 private:
  mutable std::shared_mutex rwLock;
  mutable std::mutex mtx;
  mutable std::condition_variable cv;
  mutable int active_readers = 0;
  int active_writers = 0;
  int waiting_writers = 0;

 public:
  FairRWLock() = default;
  FairRWLock(FairRWLock &&other) noexcept;
  void acquire_read() const;
  void release_read() const;
  void acquire_write();
  void release_write();
};
}  // namespace cpputils
