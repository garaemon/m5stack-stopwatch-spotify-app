#pragma once

#include <deque>
#include <mutex>
#include <optional>
#include <utility>

// FIFO queue that one FreeRTOS task can push to while another pops.
// Never blocks the popping side; callers poll with tryPop().
template <typename T>
class ThreadSafeQueue {
 public:
  void push(T value) {
    const std::lock_guard<std::mutex> lock(mutex_);
    items_.push_back(std::move(value));
  }

  std::optional<T> tryPop() {
    const std::lock_guard<std::mutex> lock(mutex_);
    if (items_.empty()) {
      return std::nullopt;
    }
    std::optional<T> frontItem(std::move(items_.front()));
    items_.pop_front();
    return frontItem;
  }

 private:
  std::mutex mutex_;
  std::deque<T> items_;
};
