#include "Owned.h"
#include <memory>
#include <mutex>
#include <shared_mutex>

template <typename T> class ThreadSafeContainer {
  static_assert(std::is_base_of_v<Owned, T>, "T must be an Owned type");

private:
  std::shared_ptr<T> data;
  std::shared_ptr<std::shared_mutex> rwLock;

public:
  ThreadSafeContainer(T &&initialData)
      : data(std::make_shared<T>(std::move(initialData))),
        rwLock(std::make_shared<std::shared_mutex>()) {}

  ThreadSafeContainer(const ThreadSafeContainer &other)
      : data(other.data), rwLock(other.rwLock) {}

  template <typename Func>
  auto read(Func func) const -> decltype(func(std::declval<const T &>())) {
    std::shared_lock lock(*rwLock);
    return func(*data);
  }

  template <typename Func> auto write(Func func) -> decltype(func(*data)) {
    std::unique_lock lock(*rwLock);
    return func(*data);
  }
};
