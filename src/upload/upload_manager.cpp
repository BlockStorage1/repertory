/*
  Copyright <2018-2022> <scott.e.graves@protonmail.com>

  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
  associated documentation files (the "Software"), to deal in the Software without restriction,
  including without limitation the rights to use, copy, modify, merge, publish, distribute,
  sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all copies or
  substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
  NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
  DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT
  OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
#include "upload/upload_manager.hpp"
#include "comm/i_comm.hpp"
#include "app_config.hpp"
#include "events/events.hpp"
#include "events/event_system.hpp"
#include "utils/file_utils.hpp"
#include "utils/rocksdb_utils.hpp"

namespace repertory {
upload_manager::upload_manager(const app_config &config, api_file_exists_callback api_file_exists,
                               upload_handler_callback upload_handler,
                               upload_completed_callback upload_completed)
    : config_(config),
      api_file_exists_(std::move(api_file_exists)),
      upload_handler_(std::move(upload_handler)),
      upload_completed_(std::move(upload_completed)) {
  utils::db::create_rocksdb(config, ROCKS_DB_NAME, upload_db_);
  auto iterator =
      std::unique_ptr<rocksdb::Iterator>(upload_db_->NewIterator(rocksdb::ReadOptions()));
  for (iterator->SeekToFirst(); iterator->Valid(); iterator->Next()) {
    auto data = json::parse(iterator->value().ToString());
    auto u = std::make_shared<upload>(upload{
        data["path"].get<std::string>(),
        false,
        data["token"].get<std::string>(),
        data["source"].get<std::string>(),
    });

    upload_lookup_[iterator->key().ToString()] = u;
    upload_queue_.emplace_back(u.get());
  }
}

upload_manager::~upload_manager() {
  stop();
  upload_db_.reset();
}

bool upload_manager::execute_if_not_processing(const std::vector<std::string> &api_paths,
                                               const std::function<void()> &action) const {
  mutex_lock upload_lock(upload_mutex_);
  const auto ret =
      (std::find_if(api_paths.begin(), api_paths.end(), [this](const auto &path) -> bool {
         return upload_lookup_.find(path) != upload_lookup_.end();
       }) == api_paths.end());
  if (ret) {
    action();
  }

  return ret;
}

bool upload_manager::is_processing(const std::string &api_path) const {
  mutex_lock upload_lock(upload_mutex_);
  return upload_lookup_.find(api_path) != upload_lookup_.end();
}

api_error upload_manager::queue_upload(const std::string &api_path, const std::string &source_path,
                                       const std::string &encryption_token) {
  auto ret = remove_upload(api_path);
  if ((ret == api_error::success) || (ret == api_error::item_not_found)) {
    ret = api_error::success;

    auto u = std::make_shared<upload>(upload{api_path, false, encryption_token, source_path});

    unique_mutex_lock upload_lock(upload_mutex_);
    upload_db_->Put(rocksdb::WriteOptions(), api_path,
                    json({
                             {"path", api_path},
                             {"source", source_path},
                             {"token", encryption_token},
                         })
                        .dump());
    if (not stop_requested_) {
      upload_lookup_[api_path] = u;
      upload_queue_.emplace_back(u.get());
    }
    upload_notify_.notify_all();
    event_system::instance().raise<file_upload_queued>(api_path, source_path);
    upload_lock.unlock();
  }

  return ret;
}

api_error upload_manager::remove_upload(const std::string &api_path) {
  auto ret = api_error::item_not_found;

  unique_mutex_lock upload_lock(upload_mutex_);
  if (upload_lookup_.find(api_path) != upload_lookup_.end()) {
    auto upload = upload_lookup_[api_path];
    utils::remove_element_from(upload_queue_, upload.get());
    upload_lookup_.erase(api_path);
    upload_db_->Delete(rocksdb::WriteOptions(), api_path);
    upload->cancel = true;
    upload_lock.unlock();

    upload->wait();

    event_system::instance().raise<file_upload_removed>(api_path, upload->source_path);
    ret = api_error::success;
  }

  return ret;
}

void upload_manager::start() {
  mutex_lock upload_lock(upload_mutex_);
  if (not upload_thread_) {
    stop_requested_ = false;
    upload_thread_ = std::make_unique<std::thread>([this]() { this->upload_thread(); });
  }
}

void upload_manager::stop() {
  unique_mutex_lock upload_lock(upload_mutex_);
  if (upload_thread_) {
    stop_requested_ = true;
    upload_notify_.notify_all();
    upload_lock.unlock();

    upload_thread_->join();
    upload_thread_.reset();
    stop_requested_ = false;

    event_system::instance().raise<service_shutdown>("upload_manager");
  }
}

void upload_manager::upload_thread() {
  unique_mutex_lock upload_lock(upload_mutex_);
  upload_lock.unlock();

  std::deque<std::shared_ptr<upload>> completed;
  const auto process_completed = [&]() {
    while (not completed.empty()) {
      upload_lock.lock();
      auto u = completed.front();
      completed.pop_front();
      upload_lock.unlock();

      u->thread->join();
      u->thread.reset();
      u->completed = true;
    }
  };

  const auto process_next = [this, &completed]() {
    if (not upload_queue_.empty()) {
      auto *next_upload = upload_queue_.front();
      if (not next_upload->thread) {
        upload_queue_.pop_front();
        next_upload->completed = false;
        active_uploads_.emplace_back(next_upload);
        auto u = upload_lookup_[next_upload->api_path];
        auto upload_handler = [this, &completed, u]() {
          json data, err;
          const auto ret = this->upload_handler_(*u, data, err);

          unique_mutex_lock upload_lock(upload_mutex_);
          if (ret == api_error::success) {
            upload_lookup_.erase(u->api_path);
            upload_db_->Delete(rocksdb::WriteOptions(), u->api_path);
            upload_completed_(u->api_path, u->source_path, data);
            event_system::instance().raise<file_upload_completed>(u->api_path, u->source_path);
          } else {
            event_system::instance().raise<file_upload_failed>(u->api_path, u->source_path,
                                                               err.dump(2));
            if (u->cancel) {
              upload_lookup_.erase(u->api_path);
            } else if (not api_file_exists_(u->api_path) ||
                       not utils::file::is_file(u->source_path)) {
              upload_lookup_.erase(u->api_path);
              upload_db_->Delete(rocksdb::WriteOptions(), u->api_path);
              event_system::instance().raise<file_upload_not_found>(u->api_path, u->source_path);
            } else {
              upload_queue_.emplace_back(u.get());
              u->retry = true;
            }
          }
          utils::remove_element_from(active_uploads_, u.get());

          completed.emplace_back(u);
          upload_notify_.notify_all();
          upload_lock.unlock();
        };

        u->thread = std::make_unique<std::thread>(std::move(upload_handler));
      }
    }
  };

  const auto has_only_retries = [this]() -> bool {
    return std::find_if(upload_queue_.begin(), upload_queue_.end(), [](const auto &upload) -> bool {
             return not upload->retry;
           }) == upload_queue_.end();
  };

  const auto cancel_all_active = [this, &process_completed, &upload_lock]() {
    upload_lock.lock();
    for (auto &upload : upload_lookup_) {
      upload.second->cancel = true;
    }
    upload_lock.unlock();

    process_completed();
  };

  const auto drain_queue = [this, &process_completed, &process_next, &upload_lock]() {
    while (not upload_lookup_.empty()) {
      upload_lock.lock();
      process_next();
      upload_lock.unlock();

      process_completed();
    }
  };

  const auto run_queue_loop = [this, &has_only_retries, &process_completed, &process_next,
                               &upload_lock]() {
    while (not stop_requested_) {
      upload_lock.lock();
      if ((active_uploads_.size() >= config_.get_max_upload_count()) || upload_queue_.empty()) {
        upload_notify_.wait_for(upload_lock, 5s);
        if (not stop_requested_ && has_only_retries()) {
          upload_notify_.wait_for(upload_lock, 5s);
        }
      } else {
        process_next();
      }
      upload_lock.unlock();

      process_completed();
    }
  };

  run_queue_loop();
  cancel_all_active();
  drain_queue();
}
} // namespace repertory
