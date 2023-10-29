#include "utils/action_queue.hpp"

#include "types/repertory.hpp"

namespace repertory::utils::action_queue {
action_queue::action_queue(const std::string &id,
                           std::uint8_t max_concurrent_actions)
    : single_thread_service_base("action_queue_" + id),
      id_(id),
      max_concurrent_actions_(max_concurrent_actions) {}

void action_queue::service_function() {
  //
}

void action_queue::push(std::function<void()> action) {
  unique_mutex_lock queue_lock(queue_mtx_);
  queue_.emplace_back(action);
  queue_notify_.notify_all();
}
} // namespace repertory::utils::action_queue
