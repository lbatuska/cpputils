#pragma once

#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <optional>
#include <queue>
#include <utility>

namespace cpputils {
template <typename T>
class SafeQueue {
 private:
  std::queue<T> queue;
  mutable std::mutex mtx;
  std::condition_variable cv;
  bool _closed;  // push returns false if closed

 public:
  const size_t max_size;

  SafeQueue(SafeQueue&& other) noexcept
      : queue(std::move(other.queue)),
        _closed(other._closed),
        max_size(other.max_size) {}

  SafeQueue& operator=(SafeQueue&& other) noexcept {
    if (this != &other) {
      std::lock_guard<std::mutex> lock(other.mtx);
      _closed = other._closed;
      queue = std::move(other.queue);
    }
    return *this;
  }

  explicit SafeQueue(size_t MaxSize) noexcept(true)
      : _closed(false), max_size(MaxSize) {}

  bool push(const T& item) noexcept {
    std::unique_lock<std::mutex> lock(mtx);
    if (_closed)  // We should not be able to push into a closed queue
      return false;
    cv.wait(lock, [this]() {
      return queue.size() < max_size;
    });  // Wait if the queue is full
    queue.push(item);
    cv.notify_one();
    return true;
  }

  bool push(T&& item) noexcept {
    std::unique_lock<std::mutex> lock(mtx);
    if (_closed)
      return false;
    cv.wait(lock, [this]() { return queue.size() < max_size; });
    queue.push(std::move(item));
    cv.notify_one();
    return true;
  }

  // Calling pop on a closed SafeQueue is UB (or if pop is waiting on an empty
  // SafeQueue that get's closed) -> use popsafe
  [[nodiscard]] T pop() {
    std::unique_lock<std::mutex> lock(mtx);
    cv.wait(lock,
            [this]() { return !queue.empty(); });  // Wait if the queue is empty
    T item = std::move(queue.front());
    queue.pop();
    cv.notify_all();
    return item;
  }

  // Being woken up by closing the queue when it is empty returns a std::nullopt
  [[nodiscard]] std::optional<T> popsafe() noexcept {
    std::unique_lock<std::mutex> lock(mtx);
    cv.wait(lock, [this]() {
      return !queue.empty() || _closed;
    });  // Wait if the queue is empty
    if (_closed && queue.empty()) {
      return std::nullopt;
    }
    T item = std::move(queue.front());
    queue.pop();
    cv.notify_all();
    return std::optional<T>(std::move(item));
  }

  // Peeking a closed SafeQueue is UB
  template <typename CloneType = T>
  [[nodiscard]] auto peek() const noexcept -> decltype(auto) {
    std::unique_lock<std::mutex> lock(mtx);
    cv.wait(lock, [this]() { return !queue.empty(); });
    if constexpr (std::is_copy_constructible<T>::value) {
      return queue.front();
    }
    static_assert(std::is_copy_constructible<T>::value,
                  "T must be copyable or Cloneable");
  }

  template <typename CloneType = T>
  [[nodiscard]] auto peeksafe() const noexcept -> decltype(auto) {
    std::unique_lock<std::mutex> lock(mtx);
    if constexpr (std::is_copy_constructible<T>::value) {
      if (queue.empty()) {
        return std::optional<T>(std::nullopt);
      }
      return std::optional<T>(queue.front());
    }
    static_assert(std::is_copy_constructible<T>::value,
                  "T must be copyable or Cloneable");
  }

  inline bool empty() const {
    std::lock_guard<std::mutex> lock(mtx);
    return queue.empty();
  }

  inline bool closed() const {
    std::lock_guard<std::mutex> lock(mtx);
    return _closed;
  }

  // Will notify_all
  inline void close() {
    std::lock_guard<std::mutex> lock(mtx);
    _closed = true;
    cv.notify_all();
  }

  // Waits on an empty open SafeQueue
  inline void waititem() {
    std::unique_lock<std::mutex> lock(mtx);
    cv.wait(lock, [this]() { return !queue.empty() || _closed; });
  }

  inline size_t current_size() const {
    std::lock_guard<std::mutex> lock(mtx);
    return queue.size();
  }

  inline bool full() const {
    std::lock_guard<std::mutex> lock(mtx);
    return max_size <= queue.size();
  }
};
}  // namespace cpputils
