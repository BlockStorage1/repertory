#ifndef REPERTORY_INCLUDE_UTILS_TTL_CACHE_HPP_
#define REPERTORY_INCLUDE_UTILS_TTL_CACHE_HPP_

#include "utils/config.hpp"

namespace repertory::utils {
template <typename data_t, template <typename> class atomic_t = std::atomic>
class ttl_cache final {
public:
  using clock = std::chrono::steady_clock;
  using duration = std::chrono::milliseconds;
  using entry_t = atomic_t<data_t>;
  using entry_ptr_t = std::shared_ptr<entry_t>;

  static constexpr auto default_expiration{duration(60000U)};

private:
  struct entry final {
    entry_ptr_t data;
    clock::time_point expires_at;
  };

public:
  ttl_cache(duration ttl = default_expiration) : ttl_{ttl} {}

private:
  duration ttl_;

private:
  mutable std::mutex mutex_;
  std::unordered_map<std::string, entry> entries_;

public:
  void clear() {
    mutex_lock lock(mutex_);
    entries_.clear();
  }

  void erase(const std::string &api_path) {
    mutex_lock lock(mutex_);
    entries_.erase(api_path);
  }

  [[nodiscard]] auto contains(const std::string &api_path) -> bool {
    mutex_lock lock(mutex_);
    return entries_.contains(api_path);
  }

  [[nodiscard]] auto get(const std::string &api_path) -> entry_ptr_t {
    mutex_lock lock(mutex_);
    auto iter = entries_.find(api_path);
    if (iter == entries_.end()) {
      return nullptr;
    }

    iter->second.expires_at = clock::now() + ttl_;
    return iter->second.data;
  }

  void purge_expired() {
    mutex_lock lock(mutex_);
    auto now = clock::now();
    for (auto iter = entries_.begin(); iter != entries_.end();) {
      if (iter->second.expires_at <= now) {
        iter = entries_.erase(iter);
        continue;
      }

      ++iter;
    }
  }

  [[nodiscard]] auto get_ttl() const -> duration {
    mutex_lock lock(mutex_);
    return ttl_;
  }

  void set(const std::string &api_path, const data_t &data) {
    mutex_lock lock(mutex_);
    if (entries_.contains(api_path)) {
      auto &entry = entries_.at(api_path);
      entry.data->store(data);
      entry.expires_at = clock::now() + ttl_;
      return;
    }

    entries_.emplace(api_path, entry{
                                   .data = std::make_shared<entry_t>(data),
                                   .expires_at = clock::now() + ttl_,
                               });
  }

  void set_ttl(duration ttl) {
    mutex_lock lock(mutex_);
    ttl_ = ttl;
  }
};
} // namespace repertory::utils

#endif // REPERTORY_INCLUDE_UTILS_TTL_CACHE_HPP_
