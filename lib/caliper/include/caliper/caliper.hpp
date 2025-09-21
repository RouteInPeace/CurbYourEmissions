#pragma once

#include <functional>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <vector>

namespace cali {

template <typename Config, typename Result>
class Caliper {
 public:
  struct Task {
    Config config;
    size_t samples_left;
    std::vector<Result> results;
  };

  Caliper(std::function<Result(Config)> measurement_function);

  auto add_measurement(size_t sample_size, Config config) -> void;
  auto run(size_t num_threads) -> std::vector<Task>;

 private:
  auto worker_() -> void;

  std::vector<Task> tasks_;
  size_t current_task_;
  std::mutex task_mutex_;

  bool measurement_done_;

  std::vector<std::thread> threads_;
  std::function<Result(Config const &)> measurement_function_;
};

template <typename Config, typename Result>
Caliper<Config, Result>::Caliper(std::function<Result(Config)> measurement_function)
    : current_task_(0), measurement_done_(false), measurement_function_(measurement_function) {}

template <typename Config, typename Result>
auto Caliper<Config, Result>::add_measurement(size_t sample_size, Config config) -> void {
  if (measurement_done_) {
    throw std::runtime_error("Measured again without reseting");
  }

  tasks_.emplace_back(std::move(config), sample_size, std::vector<Result>{});
}

template <typename Config, typename Result>
auto Caliper<Config, Result>::worker_() -> void {
  while (true) {
    std::unique_lock lock(task_mutex_);
    if (current_task_ < tasks_.size() && tasks_[current_task_].samples_left == 0) {
      current_task_++;
    }

    if (current_task_ >= tasks_.size()) {
      break;
    }

    auto &task = tasks_[current_task_];
    task.samples_left--;
    lock.unlock();

    auto result = measurement_function_(task.config);
    lock.lock();
    task.results.push_back(result);
    lock.unlock();
  }
}

template <typename Config, typename Result>
auto Caliper<Config, Result>::run(size_t num_threads) -> std::vector<Task> {
  for (auto i = 0UZ; i < num_threads; ++i) {
    threads_.emplace_back(&Caliper::worker_, this);
  }

  for (auto &th : threads_) th.join();
  measurement_done_ = true;

  return std::move(tasks_);
}

}  // namespace cali