#pragma once
#include "Owned.h"
#include "SafeQueue.h"
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <thread>
#include <vector>

namespace cpputils {
class TaskScheduler : public Owned {
private:
  SafeQueue<std::function<void()>> taskQueue;
  std::vector<std::thread> workerThreads;
  std::atomic<bool> isRunning;
  const size_t numThreads;
  std::vector<uint64_t> threadStartTimestamps;
  std::function<void(size_t)> taskDoneCallback;
  std::mutex callbackMutex;

private:
  void workerFunction(size_t threadId);

public:
  TaskScheduler(size_t numThreads, size_t queueSize);
  TaskScheduler(TaskScheduler &&other) noexcept;
  ~TaskScheduler() noexcept;
  bool addTask(std::function<void()> &&task) noexcept;
  void stop() noexcept;
  void waitForCompletion() const noexcept;

  // size_t is the threadId of the thread that finished with a task
  // in the range [0 -> numThreads]
  void setTaskDoneCallback(std::function<void(size_t)> callback);

public:
  inline size_t getNumThreads() const { return numThreads; }
  inline bool running() const { return isRunning; }
  inline size_t queueSize() const { return taskQueue.current_size(); }

  inline uint64_t getThreadStartTimestamp(size_t threadId) const {
    if (threadId < numThreads) {
      return threadStartTimestamps[threadId];
    }
    return 0;
  }

  inline std::vector<uint64_t> const getThreadStartTimestamps() const {
    return threadStartTimestamps;
  }
};
} // namespace cpputils
