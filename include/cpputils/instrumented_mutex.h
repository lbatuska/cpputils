#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <thread>

#ifdef SPDLOG_ACTIVE_LEVEL
#include <spdlog/spdlog.h>
#include <sstream>
#include <string>

// Custom formatter for std::thread::id
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

// https://en.cppreference.com/w/cpp/named_req/Mutex
class instrumented_mutex : public std::mutex {
#ifdef DEBUG

 private:
  int64_t locked_at;
  std::atomic<std::thread::id> holder;

 public:
  void lock() {
    auto self = std::this_thread::get_id();
    auto curr_holder = holder.load();

    if (curr_holder == self) {
#ifdef SPDLOG_ACTIVE_LEVEL
      SPDLOG_LOGGER_WARN(
          spdlog::default_logger(),
          "[Recursive Locking] Recursive lock attempt by thread {}", self);
#else
      std::cerr << "[Recursive Locking] Recursive lock attempt by thread "
                << self << "\n";
#endif
      std::terminate();
    }

    if (!std::mutex::try_lock() && curr_holder != std::thread::id()) {
#ifdef SPDLOG_ACTIVE_LEVEL
      SPDLOG_LOGGER_TRACE(spdlog::default_logger(),
                          "[Lock Contention] Thread {}  waiting for "
                          "lock, held by Thread {}",
                          self, curr_holder);
#else
      std::cout << "Lock Contention] Thread " << self
                << " waiting for lock, held by Thread " << curr_holder << "\n";
#endif
      std::mutex::lock();
    }

    holder = self;
    locked_at = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch())
                    .count();
  }

  void unlock() {
    auto self = std::this_thread::get_id();
    auto curr_holder = holder.load();
    if (curr_holder == std::thread::id()) {
#ifdef SPDLOG_ACTIVE_LEVEL
      SPDLOG_LOGGER_WARN(spdlog::default_logger(),
                         "[Unlocking non-locked mutex] Unlock "
                         "called by {} ,but mutex is not locked.",
                         self);
#else
      std::cerr << "[Unlocking non-locked mutex] Unlock called by" << self
                << " ,but mutex is not locked.\n";
#endif
      std::terminate();
    }
    if (curr_holder != self) {
#ifdef SPDLOG_ACTIVE_LEVEL
      SPDLOG_LOGGER_WARN(spdlog::default_logger(),
                         "[Unlocking non-owned mutex] Thread {} "
                         "tried to unlock mutex held by {}",
                         self, curr_holder);
#else
      std::cerr << "[Unlocking non-owned mutex] Thread " << self
                << " tried to unlock mutex held by " << curr_holder << "\n";
#endif
      std::terminate();
    }
    auto now = std::chrono::system_clock::now().time_since_epoch();
    auto duration_ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(now).count() -
        locked_at;
    (void)duration_ms;
#ifdef SPDLOG_ACTIVE_LEVEL
    SPDLOG_LOGGER_TRACE(spdlog::default_logger(),
                        "[Unlocked] Thread {} held lock for {} ms", self,
                        duration_ms);
#else
    std::cout << "[Unlocked] Thread " << self << " held lock for "
              << duration_ms << " ms\n";
#endif

    holder = std::thread::id{};
    std::mutex::unlock();
  }

  bool try_lock() {
    auto self = std::this_thread::get_id();
    auto curr_holder = holder.load();

    if (curr_holder == self) {
#ifdef SPDLOG_ACTIVE_LEVEL
      SPDLOG_LOGGER_WARN(
          spdlog::default_logger(),
          "[Recursive Locking] Recursive try_lock attempt by thread {}", self);
#else
      std::cerr << "[Recursive Locking] Recursive try_lock attempt by thread "
                << self << "\n";
#endif
      std::terminate();
    }

    if (std::mutex::try_lock()) {
      holder = self;
      locked_at = std::chrono::duration_cast<std::chrono::milliseconds>(
                      std::chrono::system_clock::now().time_since_epoch())
                      .count();
      return true;
    }

    if (curr_holder != std::thread::id{}) {
#ifdef SPDLOG_ACTIVE_LEVEL
      SPDLOG_LOGGER_TRACE(
          spdlog::default_logger(),
          "[Lock Contention] Thread {} tried to acquire (try_lock) "
          "lock held by Thread {}",
          self, curr_holder);
#else
      std::cout << "[Lock Contention] Thread " << self
                << " failed to acquire (try_lock) lock held by Thread "
                << curr_holder << "\n";
#endif
    }

    return false;
  }

  bool locked_by_caller() const { return holder == std::this_thread::get_id(); }

  bool is_locked() const { return holder != std::thread::id(); }

#endif  // DEBUG
};
}  // namespace cpputils
