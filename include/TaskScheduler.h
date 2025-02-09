#pragma once
#include "Owned.h"
#include "SafeQueue.h"
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iostream>
#include <ostream>
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
      auto task = maybetask.value();
      if (task) {
        threadStartTimestamps[threadId] =
            std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now().time_since_epoch())
                .count();
        try {
          task();
        } catch (const std::exception &e) {
          std::cerr << "Caught std::exception: " << e.what() << "\n";
        } catch (...) {
          std::cerr << "Caught unknown exception\n";
        }
        std::lock_guard<std::mutex> lock(callbackMutex);
        if (taskDoneCallback) {
          taskDoneCallback(threadId);
        }
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
      : taskQueue(std::move(other.taskQueue)), numThreads(other.numThreads) {};

  // template <typename Func> void addTask(Func &&task) {
  //   taskQueue.push(std::forward<Func>(task));
  // }

  inline bool addTask(std::function<void()> &&task) {
    return taskQueue.push(std::move(task));
  }

  inline size_t getNumThreads() const { return numThreads; }

  void stop() {
    isRunning = false;
    taskQueue.close();
    for (auto &thread : workerThreads) {
      if (thread.joinable()) {
        thread.join();
      }
    }
  }

  void waitForCompletion() const {
    while (!taskQueue.empty()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
  }

  ~TaskScheduler() {
    if (isRunning) {
      stop();
    }
  }

  // size_t is the threadId of the thread that finished with a task
  // in the range [0 -> numThreads]
  void setTaskDoneCallback(std::function<void(size_t)> callback) {
    std::lock_guard<std::mutex> lock(callbackMutex);
    taskDoneCallback = std::move(callback);
  }

  uint64_t getThreadStartTimestamp(size_t threadId) const {
    if (threadId < numThreads) {
      return threadStartTimestamps[threadId];
    }
    return 0;
  }

  inline std::vector<uint64_t> const getThreadStartTimestamps() const {
    return threadStartTimestamps;
  }

  inline bool running() const { return isRunning; }

  inline size_t queueSize() const { return taskQueue.current_size(); }
};
} // namespace cpputils
