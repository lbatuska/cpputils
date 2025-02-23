#pragma once
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <future>
#include <iostream>
#include <optional>
#include <thread>
#include <vector>

#include "Owned.h"
#include "SafeQueue.h"

namespace cpputils {

template <typename T = void>
class TaskScheduler : public Owned {
 private:
  using TaskType =
      std::conditional_t<std::is_void<T>::value, std::function<void()>,
                         std::packaged_task<T()>>;

  using QueueType = std::conditional_t<std::is_void<T>::value,
                                       SafeQueue<std::function<void()>>,
                                       SafeQueue<std::packaged_task<T()>>>;

  QueueType taskQueue;
  std::vector<std::thread> workerThreads;
  std::atomic<bool> isRunning;
  const size_t numThreads;
  std::vector<uint64_t> threadStartTimestamps;
  std::conditional_t<std::is_void<T>::value, std::function<void(size_t)>,
                     std::function<void(size_t, std::optional<T>)>>
      taskDoneCallback;
  std::mutex callbackMutex;

 private:
  void workerFunction(size_t threadId) {
    while (isRunning || !taskQueue.empty()) {
      if (taskQueue.empty()) {
        taskQueue.waititem();
        if (!isRunning && taskQueue.empty()) {
          return;
        }
      }
      auto maybetask = taskQueue.popsafe();
      if (!maybetask.has_value()) {
        return;
      }
      auto &task = maybetask.value();
      try {
        threadStartTimestamps[threadId] =
            std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now().time_since_epoch())
                .count();
        if constexpr (std::is_void<T>::value) {
          task();
          if (taskDoneCallback) {
            std::lock_guard<std::mutex> lock(callbackMutex);
            taskDoneCallback(threadId);
          }
        } else {
          task();
          if (taskDoneCallback) {
            std::future<T> f = task.get_future();
            std::lock_guard<std::mutex> lock(callbackMutex);
            taskDoneCallback(threadId, std::move(f.get()));
          }
        }
      } catch (const std::exception &e) {
        std::cerr << "Caught std::exception: " << e.what() << "\n";
      } catch (...) {
        std::cerr << "Caught unknown exception\n";
      }
    }
  }

 public:
  TaskScheduler(size_t numThreads, size_t queueSize)
      : taskQueue(queueSize), isRunning(true), numThreads(numThreads) {
    threadStartTimestamps.resize(numThreads);
    for (size_t i = 0; i < numThreads; ++i) {
      workerThreads.emplace_back([this, i]() { this->workerFunction(i); });
    }
  }

  TaskScheduler(TaskScheduler &&other) noexcept
      : taskQueue(std::move(other.taskQueue)),
        workerThreads(std::move(other.workerThreads)),
        isRunning(other.isRunning.load()),
        numThreads(other.numThreads),
        threadStartTimestamps(std::move(other.threadStartTimestamps)),
        taskDoneCallback(std::move(other.taskDoneCallback)),
        callbackMutex() {}

  ~TaskScheduler() noexcept {
    if (isRunning) {
      stop();
    }
  }

  bool addTask(TaskType &&task) noexcept {
    return taskQueue.push(std::move(task));
  }

  void stop() noexcept {
    isRunning = false;
    taskQueue.close();
    for (auto &thread : workerThreads) {
      if (thread.joinable()) {
        thread.join();
      }
    }
  }

  void waitForCompletion() const noexcept {
    while (!taskQueue.empty()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
  }

  void setTaskDoneCallback(
      std::conditional_t<std::is_void<T>::value, std::function<void(size_t)>,
                         std::function<void(size_t, std::optional<T>)>>
          callback) {
    std::lock_guard<std::mutex> lock(callbackMutex);
    taskDoneCallback = std::move(callback);
  }

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

}  // namespace cpputils
