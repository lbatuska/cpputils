#pragma once

#include "Owned.h"
#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <queue>
#include <utility>

template <typename T> class SafeQueue : public Owned {
private:
  std::queue<T> queue;
  mutable std::mutex mtx;
  std::condition_variable cv;
  bool _closed; // push returns false if closed

public:
  const size_t max_size;

  explicit SafeQueue(size_t size) noexcept(true)
      : max_size(size), _closed(false) {}

  bool push(const T &item) {
    std::unique_lock<std::mutex> lock(mtx);
    if (_closed) // We should not be able to push into a closed queue
      return false;
    cv.wait(lock, [this]() {
      return queue.size() < max_size;
    }); // Wait if the queue is full
    queue.push(item);
    cv.notify_all();
    return true;
  }

  bool push(T &&item) {
    std::unique_lock<std::mutex> lock(mtx);
    if (_closed)
      return false;
    cv.wait(lock, [this]() { return queue.size() < max_size; });
    queue.push(std::move(item));
    cv.notify_all();
    return true;
  }

  [[nodiscard]] T pop() {
    std::unique_lock<std::mutex> lock(mtx);
    cv.wait(lock,
            [this]() { return !queue.empty(); }); // Wait if the queue is empty
    T item = std::move(queue.front());
    queue.pop();
    cv.notify_all();
    return item;
  }

  const inline bool empty() const {
    std::lock_guard<std::mutex> lock(mtx);
    return queue.empty();
  }

  const inline bool closed() const {
    std::lock_guard<std::mutex> lock(mtx);
    return _closed;
  }

  inline void close() {
    std::lock_guard<std::mutex> lock(mtx);
    _closed = true;
  }
  const inline size_t current_size() const {
    std::lock_guard<std::mutex> lock(mtx);
    return queue.size();
  }
};
