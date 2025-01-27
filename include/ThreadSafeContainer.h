#include <memory>
#include <mutex>
#include <shared_mutex>

template <typename T> class ThreadSafeContainer {
private:
  std::shared_ptr<T> data;
  std::shared_ptr<std::shared_mutex> rwLock;

public:
  ThreadSafeContainer(T &&initialData)
      : data(std::make_shared<T>(std::move(initialData))),
        rwLock(std::make_shared<std::shared_mutex>()) {}

  ThreadSafeContainer(const ThreadSafeContainer &other)
      : data(other.data), rwLock(other.rwLock) {}

  template <typename Func> auto read(Func func) const -> decltype(func(*data)) {
    std::shared_lock lock(*rwLock);
    return func(*data);
  }

  template <typename Func> auto write(Func func) -> decltype(func(*data)) {
    std::unique_lock lock(*rwLock);
    return func(*data);
  }
};
