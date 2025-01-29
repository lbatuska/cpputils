#pragma once

#include "Owned.h"
#include <condition_variable>
#include <mutex>
#include <shared_mutex>

class FairRWLock : Owned {
private:
  mutable std::shared_mutex rwLock;
  mutable std::mutex mtx;
  mutable std::condition_variable cv;
  mutable int active_readers = 0;
  int active_writers = 0;
  int waiting_writers = 0;

public:
  void acquire_read() const {
    std::unique_lock<std::mutex> lock(mtx);
    cv.wait(lock,
            [this]() { return active_writers == 0 && waiting_writers == 0; });
    ++active_readers;
    rwLock.lock_shared();
  }

  void release_read() const {
    rwLock.unlock_shared();
    std::unique_lock<std::mutex> lock(mtx);
    if (--active_readers == 0) {
      cv.notify_all();
    }
  }

  void acquire_write() {
    std::unique_lock<std::mutex> lock(mtx);
    ++waiting_writers;
    cv.wait(lock,
            [this]() { return active_readers == 0 && active_writers == 0; });
    --waiting_writers;
    ++active_writers;
    rwLock.lock();
  }

  void release_write() {
    rwLock.unlock();
    std::unique_lock<std::mutex> lock(mtx);
    --active_writers;
    cv.notify_all();
  }
};
