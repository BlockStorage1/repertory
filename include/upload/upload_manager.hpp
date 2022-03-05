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
#ifndef INCLUDE_UPLOAD_UPLOAD_MANAGER_HPP_
#define INCLUDE_UPLOAD_UPLOAD_MANAGER_HPP_

#include "common.hpp"
#include "types/repertory.hpp"

namespace repertory {
class app_config;
class upload_manager final {
public:
  struct upload {
    std::string api_path;
    bool cancel = false;
    std::string encryption_token;
    std::string source_path;
    std::unique_ptr<std::thread> thread;
    bool completed = true;
    bool retry = false;

    void wait() {
      while (not completed) {
        std::this_thread::sleep_for(1ms);
      }
    }
  };

  typedef std::function<bool(const std::string &api_path)> api_file_exists_callback;
  typedef std::function<host_config(const bool &upload)> get_host_config_callback;
  typedef std::function<void(const std::string &api_path, const std::string &source_path,
                             const json &data)>
      upload_completed_callback;
  typedef std::function<api_error(const upload &upload, json &data, json &error)>
      upload_handler_callback;

public:
  upload_manager(const app_config &config, api_file_exists_callback api_file_exists,
                 upload_handler_callback upload_handler,
                 upload_completed_callback upload_completed);

  ~upload_manager();

private:
  const app_config &config_;
  const api_file_exists_callback api_file_exists_;
  upload_handler_callback upload_handler_;
  const upload_completed_callback upload_completed_;
  std::unique_ptr<rocksdb::DB> upload_db_;
  std::unordered_map<std::string, std::shared_ptr<upload>> upload_lookup_;
  std::deque<upload *> upload_queue_;
  std::deque<upload *> active_uploads_;
  const std::string ROCKS_DB_NAME = "upload_db";
  bool stop_requested_ = false;
  mutable std::mutex upload_mutex_;
  std::condition_variable upload_notify_;
  std::unique_ptr<std::thread> upload_thread_;

private:
  void upload_thread();

public:
  bool execute_if_not_processing(const std::vector<std::string> &api_paths,
                                 const std::function<void()> &action) const;

  std::size_t get_count() const { return upload_lookup_.size(); }

  bool is_processing(const std::string &api_path) const;

  api_error queue_upload(const std::string &api_path, const std::string &source_path,
                         const std::string &encryption_token);

  api_error remove_upload(const std::string &api_path);

  void start();

  void stop();
};
} // namespace repertory

#endif // INCLUDE_UPLOAD_UPLOAD_MANAGER_HPP_
