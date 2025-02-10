#include "FairRWLock.h"

using cpputils::FairRWLock;

FairRWLock::FairRWLock(FairRWLock &&other) noexcept
    : Owned(std::move(other)), rwLock(), mtx(), cv(),
      active_readers(other.active_readers),
      active_writers(other.active_writers),
      waiting_writers(other.waiting_writers) {}

void FairRWLock::acquire_read() const {
  std::unique_lock<std::mutex> lock(mtx);
  cv.wait(lock,
          [this]() { return active_writers == 0 && waiting_writers == 0; });
  ++active_readers;
  rwLock.lock_shared();
}

void FairRWLock::release_read() const {
  rwLock.unlock_shared();
  std::unique_lock<std::mutex> lock(mtx);
  if (--active_readers == 0) {
    cv.notify_all();
  }
}

void FairRWLock::acquire_write() {
  std::unique_lock<std::mutex> lock(mtx);
  ++waiting_writers;
  cv.wait(lock,
          [this]() { return active_readers == 0 && active_writers == 0; });
  --waiting_writers;
  ++active_writers;
  rwLock.lock();
}

void FairRWLock::release_write() {
  rwLock.unlock();
  std::unique_lock<std::mutex> lock(mtx);
  --active_writers;
  cv.notify_all();
}
