#pragma once
#include <memory>
#include <mutex>
#include <shared_mutex>

#include "FairRWLock.h"

namespace cpputils {
template <typename T, typename LockType = std::shared_mutex>
class ThreadSafeContainer {
  static_assert(std::is_same_v<LockType, std::shared_mutex> ||
                    std::is_same_v<LockType, std::mutex> ||
                    std::is_same_v<LockType, FairRWLock>,
                "ThreadSafeContainer can only be used with std::shared_mutex, "
                "std::mutex, or FairRWLock");

 private:
  std::shared_ptr<T> data;
  std::shared_ptr<LockType> rwLock;

 public:
  ThreadSafeContainer(T &&initialData)
      : data(std::make_shared<T>(std::move(initialData))),
        rwLock(std::make_shared<std::shared_mutex>()) {}

  ThreadSafeContainer(const ThreadSafeContainer &other)
      : data(other.data), rwLock(other.rwLock) {}

  ThreadSafeContainer(std::shared_ptr<T> &&existingData)
      : data(std::move(existingData)),
        rwLock(std::make_shared<std::shared_mutex>()) {}

  template <typename Func>
  auto read(Func func) const
      -> decltype(func(std::declval<const T &>())) const {
    std::shared_lock lock(*rwLock);
    if constexpr (std::is_void_v<decltype(func(*data))>) {
      func(*data);
      return;
    }
    return func(*data);
  }

  template <typename Func>
  auto write(Func func) -> decltype(func(*data)) {
    std::unique_lock lock(*rwLock);
    if constexpr (std::is_void_v<decltype(func(*data))>) {
      func(*data);
      return;
    }
    return func(*data);
  }
};

template <typename T>
class ThreadSafeContainer<T, FairRWLock> {
 private:
  std::shared_ptr<T> data;
  std::shared_ptr<FairRWLock> rwLock;

 public:
  ThreadSafeContainer(T &&initialData)
      : data(std::make_shared<T>(std::move(initialData))),
        rwLock(std::make_shared<FairRWLock>()) {}

  ThreadSafeContainer(std::shared_ptr<T> &&existingData)
      : data(std::move(existingData)), rwLock(std::make_shared<FairRWLock>()) {}

  ThreadSafeContainer(const ThreadSafeContainer &other)
      : data(other.data), rwLock(other.rwLock) {}

  template <typename Func>
  auto read(Func func) const
      -> decltype(func(std::declval<const T &>())) const {
    rwLock->acquire_read();
    if constexpr (std::is_void_v<decltype(func(*data))>) {
      func(*data);
      rwLock->release_read();
      return;
    } else {
      auto result = func(*data);
      rwLock->release_read();
      return result;
    }
  }

  template <typename Func>
  auto write(Func func) -> decltype(func(*data)) {
    rwLock->acquire_write();
    if constexpr (std::is_void_v<decltype(func(*data))>) {
      func(*data);
      rwLock->release_write();
      return;
    } else {
      auto result = func(*data);
      rwLock->release_write();
      return result;
    }
  }
};

template <typename T>
class ThreadSafeContainer<T, std::mutex> {
 private:
  std::shared_ptr<T> data;
  std::shared_ptr<std::mutex> rwLock;

 public:
  ThreadSafeContainer(T &&initialData)
      : data(std::make_shared<T>(std::move(initialData))),
        rwLock(std::make_shared<std::mutex>()) {}

  ThreadSafeContainer(const ThreadSafeContainer &other)
      : data(other.data), rwLock(other.rwLock) {}

  ThreadSafeContainer(std::shared_ptr<T> &&existingData)
      : data(std::move(existingData)), rwLock(std::make_shared<std::mutex>()) {}

  template <typename Func>
  auto read(Func func) const
      -> decltype(func(std::declval<const T &>())) const {
    std::unique_lock lock(*rwLock);
    if constexpr (std::is_void_v<decltype(func(*data))>) {
      func(*data);
      return;
    }
    return func(*data);
  }

  template <typename Func>
  auto write(Func func) -> decltype(func(*data)) {
    std::unique_lock lock(*rwLock);
    if constexpr (std::is_void_v<decltype(func(*data))>) {
      func(*data);
      return;
    }
    return func(*data);
  }
};
}  // namespace cpputils
