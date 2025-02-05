#pragma once
#include "Owned.h"
#include "SafeQueue.h"
#include <atomic>
#include <chrono>
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

public:
  TaskScheduler(size_t numThreads, size_t queueSize)
      : taskQueue(queueSize), isRunning(true), numThreads(numThreads) {
    for (size_t i = 0; i < numThreads; ++i) {
      workerThreads.emplace_back([this]() { this->workerFunction(); });
    }
  }

  void workerFunction() {
    while (isRunning || !taskQueue.empty()) {
      auto task = taskQueue.pop();
      if (task) {
        try {
          task();
        } catch (const std::exception &e) {
          std::cerr << "Caught std::exception: " << e.what() << "\n";
        } catch (...) {
          std::cerr << "Caught unknown exception\n";
        }
      }
    }
  }

  // template <typename Func> void addTask(Func &&task) {
  //   taskQueue.push(std::forward<Func>(task));
  // }

  bool addTask(std::function<void()> &&task) {
    return taskQueue.push(std::move(task));
  }

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
};
} // namespace cpputils
