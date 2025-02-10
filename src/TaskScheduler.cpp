#include "TaskScheduler.h"
#include <iostream>

using cpputils::TaskScheduler;

TaskScheduler::TaskScheduler(size_t numThreads, size_t queueSize)
    : taskQueue(queueSize), isRunning(true), numThreads(numThreads) {
  threadStartTimestamps.resize(numThreads);
  for (size_t i = 0; i < numThreads; ++i) {
    workerThreads.emplace_back([this, i]() { this->workerFunction(i); });
  }
}

TaskScheduler::TaskScheduler(TaskScheduler &&other) noexcept
    : taskQueue(std::move(other.taskQueue)),
      workerThreads(std::move(other.workerThreads)),
      isRunning(other.isRunning.load()), numThreads(other.numThreads),
      threadStartTimestamps(std::move(other.threadStartTimestamps)),
      taskDoneCallback(std::move(other.taskDoneCallback)), callbackMutex() {};

TaskScheduler::~TaskScheduler() noexcept {
  if (isRunning) {
    stop();
  }
}

void TaskScheduler::workerFunction(size_t threadId) {
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

bool TaskScheduler::addTask(std::function<void()> &&task) noexcept {
  return taskQueue.push(std::move(task));
}

void TaskScheduler::stop() noexcept {
  isRunning = false;
  taskQueue.close();
  for (auto &thread : workerThreads) {
    if (thread.joinable()) {
      thread.join();
    }
  }
}

void TaskScheduler::waitForCompletion() const noexcept {
  while (!taskQueue.empty()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
}

void TaskScheduler::setTaskDoneCallback(std::function<void(size_t)> callback) {
  std::lock_guard<std::mutex> lock(callbackMutex);
  taskDoneCallback = std::move(callback);
}
