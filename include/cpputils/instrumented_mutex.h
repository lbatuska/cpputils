#pragma once

#include <mutex>

#ifdef NO_INSTRUMENT_MUTEX

namespace cpputils {

typedef std::mutex instrumented_mutex;

#else

#include <atomic>
#include <chrono>
#include <thread>

#ifdef SPDLOG_ACTIVE_LEVEL
#include <spdlog/spdlog.h>
#include <sstream>
#include <string>

// Formatter for std::thread::id
namespace fmt {
template <>
struct formatter<std::thread::id> : formatter<std::string> {
  template <typename FormatContext>
  auto format(const std::thread::id& id, FormatContext& ctx) const {
    std::ostringstream oss;
    oss << id;
    return formatter<std::string>::format(oss.str(), ctx);
  }
};
}  // namespace fmt
#else
#include <iostream>
#endif

namespace cpputils {

class instrumented_mutex {
 public:
  instrumented_mutex() = default;
  instrumented_mutex(const instrumented_mutex&) = delete;
  instrumented_mutex& operator=(const instrumented_mutex&) = delete;

  void lock() {
    const auto self = std::this_thread::get_id();

    if (owner_.load(std::memory_order_relaxed) == self) {
      log_recursive_lock(self);
#ifdef MUTEX_TERMINATE
      std::terminate();
#endif
    }

    if (!mtx_.try_lock()) {
      log_contention(self);
      mtx_.lock();
    }

    owner_.store(self, std::memory_order_relaxed);
    locked_at_ = now_ms();
  }

  bool try_lock() {
    const auto self = std::this_thread::get_id();

    if (owner_.load(std::memory_order_relaxed) == self) {
      log_recursive_try_lock(self);
#ifdef MUTEX_TERMINATE
      std::terminate();
#endif
      return false;
    }

    if (mtx_.try_lock()) {
      owner_.store(self, std::memory_order_relaxed);
      locked_at_ = now_ms();
      return true;
    }

    return false;
  }

  void unlock() {
    const auto self = std::this_thread::get_id();
    const auto owner = owner_.load(std::memory_order_relaxed);

    if (owner == std::thread::id{}) {
      log_unlock_unlocked(self);
#ifdef MUTEX_TERMINATE
      std::terminate();
#endif
      return;
    }

    if (owner != self) {
      log_unlock_non_owner(self, owner);
#ifdef MUTEX_TERMINATE
      std::terminate();
#endif
    }

    owner_.store(std::thread::id{}, std::memory_order_relaxed);
    mtx_.unlock();
  }

  bool locked_by_caller() const {
    return owner_.load(std::memory_order_relaxed) == std::this_thread::get_id();
  }

  bool is_locked() const {
    return owner_.load(std::memory_order_relaxed) != std::thread::id{};
  }

 private:
  std::mutex mtx_;
  std::atomic<std::thread::id> owner_{};
  std::atomic<int64_t> locked_at_{0};

  static int64_t now_ms() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
  }

  static void log_recursive_lock(const std::thread::id& self) {
#ifdef SPDLOG_ACTIVE_LEVEL
    SPDLOG_LOGGER_WARN(spdlog::default_logger(),
                       "[Recursive Lock] Thread {} attempted to re-lock mutex",
                       self);
#else
    std::cerr << "[Recursive Lock] Thread " << self
              << " attempted to re-lock mutex\n";
#endif
  }

  static void log_recursive_try_lock(const std::thread::id& self) {
#ifdef SPDLOG_ACTIVE_LEVEL
    SPDLOG_LOGGER_WARN(spdlog::default_logger(),
                       "[Recursive TryLock] Thread {} attempted try_lock()",
                       self);
#else
    std::cerr << "[Recursive TryLock] Thread " << self
              << " attempted try_lock()\n";
#endif
  }

  static void log_contention(const std::thread::id& self) {
#ifdef SPDLOG_ACTIVE_LEVEL
    SPDLOG_LOGGER_TRACE(spdlog::default_logger(),
                        "[Lock Contention] Thread {} waiting for mutex", self);
#else
    std::cout << "[Lock Contention] Thread " << self << " waiting for mutex\n";
#endif
  }

  static void log_unlock_unlocked(const std::thread::id& self) {
#ifdef SPDLOG_ACTIVE_LEVEL
    SPDLOG_LOGGER_WARN(
        spdlog::default_logger(),
        "[Unlock Error] Thread {} tried to unlock an unlocked mutex", self);
#else
    std::cerr << "[Unlock Error] Thread " << self
              << " tried to unlock an unlocked mutex\n";
#endif
  }

  static void log_unlock_non_owner(const std::thread::id& self,
                                   const std::thread::id& owner) {
#ifdef SPDLOG_ACTIVE_LEVEL
    SPDLOG_LOGGER_WARN(
        spdlog::default_logger(),
        "[Unlock Error] Thread {} tried to unlock mutex owned by {}", self,
        owner);
#else
    std::cerr << "[Unlock Error] Thread " << self
              << " tried to unlock mutex owned by " << owner << "\n";
#endif
  }
};

#endif

}  // namespace cpputils
