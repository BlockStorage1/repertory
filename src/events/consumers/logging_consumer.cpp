/*
  Copyright <2018-2023> <scott.e.graves@protonmail.com>

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/
#include "events/consumers/logging_consumer.hpp"

#include "events/events.hpp"
#include "types/startup_exception.hpp"
#include "utils/error_utils.hpp"
#include "utils/file_utils.hpp"
#include "utils/path_utils.hpp"
#include "utils/unix/unix_utils.hpp"
#include "utils/utils.hpp"

namespace repertory {
logging_consumer::logging_consumer(const std::string &log_directory,
                                   const event_level &level)
    : event_level_(level),
      log_directory_(utils::path::finalize(log_directory)),
      log_path_(utils::path::combine(log_directory, {"repertory.log"})) {
  if (not utils::file::create_full_directory_path(log_directory_)) {
    throw startup_exception("failed to create log directory|sp|" +
                            log_directory_ + "|err|" +
                            std::to_string(utils::get_last_error_code()));
  }

  check_log_roll(0);
  reopen_log_file();
  E_SUBSCRIBE_ALL(process_event);
  E_SUBSCRIBE_EXACT(event_level_changed,
                    [this](const event_level_changed &evt) {
                      event_level_ = event_level_from_string(
                          evt.get_new_event_level().get<std::string>());
                    });
  logging_thread_ =
      std::make_unique<std::thread>([this] { logging_thread(false); });
}

logging_consumer::~logging_consumer() {
  E_CONSUMER_RELEASE();

  unique_mutex_lock l(log_mutex_);
  logging_active_ = false;
  log_notify_.notify_all();
  l.unlock();

  logging_thread_->join();
  logging_thread_.reset();

  logging_thread(true);
  close_log_file();
}

void logging_consumer::check_log_roll(std::size_t count) {
  std::uint64_t file_size{};
  const auto success = utils::file::get_file_size(log_path_, file_size);
  if (success && (file_size + count) >= MAX_LOG_FILE_SIZE) {
    close_log_file();
    for (std::uint8_t i = MAX_LOG_FILES; i > 0u; i--) {
      const auto temp_log_path = utils::path::combine(
          log_directory_, {"repertory." + std::to_string(i) + ".log"});
      if (utils::file::is_file(temp_log_path)) {
        if (i == MAX_LOG_FILES) {
          if (not utils::file::retry_delete_file(temp_log_path)) {
          }
        } else {
          const auto next_file_path = utils::path::combine(
              log_directory_,
              {"repertory." + std::to_string(i + std::uint8_t(1)) + ".log"});
          if (not utils::file::move_file(temp_log_path, next_file_path)) {
            utils::error::raise_error(__FUNCTION__,
                                      utils::get_last_error_code(),
                                      temp_log_path + "|dest|" + next_file_path,
                                      "failed to move file");
          }
        }
      }
    }

    auto backup_log_path =
        utils::path::combine(log_directory_, {"repertory.1.log"});
    if (not utils::file::move_file(log_path_, backup_log_path)) {
      utils::error::raise_error(__FUNCTION__, utils::get_last_error_code(),
                                log_path_ + "|dest|" + backup_log_path,
                                "failed to move file");
    }
    reopen_log_file();
  }
}

void logging_consumer::close_log_file() {
  if (log_file_) {
    fclose(log_file_);
    log_file_ = nullptr;
  }
}

void logging_consumer::logging_thread(bool drain) {
  do {
    std::deque<std::shared_ptr<event>> events;
    {
      unique_mutex_lock l(log_mutex_);
      if (event_queue_.empty() && not drain) {
        log_notify_.wait_for(l, 2s);
      } else {
        events.insert(events.end(), event_queue_.begin(), event_queue_.end());
        event_queue_.clear();
      }
    }

    while (not events.empty()) {
      auto event = events.front();
      events.pop_front();

      if (event->get_event_level() <= event_level_) {
        const std::string msg = ([&]() -> std::string {
          struct tm local_time {};
          utils::get_local_time_now(local_time);

          std::stringstream ss;
          ss << std::put_time(&local_time, "%F %T") << "|"
             << event_level_to_string(event->get_event_level()).c_str() << "|"
             << event->get_single_line().c_str() << std::endl;
          return ss.str();
        })();

        check_log_roll(msg.length());
        auto retry = true;
        for (int i = 0; retry && (i < 2); i++) {
          retry = (not log_file_ || (fwrite(&msg[0], 1, msg.length(),
                                            log_file_) != msg.length()));
          if (retry) {
            reopen_log_file();
          }
        }

        if (log_file_) {
          fflush(log_file_);
        }
      }
    }
  } while (logging_active_);
}

void logging_consumer::process_event(const event &event) {
  {
    mutex_lock l(log_mutex_);
    event_queue_.push_back(event.clone());
    log_notify_.notify_all();
  }
}

void logging_consumer::reopen_log_file() {
  close_log_file();
#ifdef _WIN32
  log_file_ = _fsopen(&log_path_[0], "a+", _SH_DENYWR);
#else
  log_file_ = fopen(&log_path_[0], "a+");
#endif
}
} // namespace repertory
