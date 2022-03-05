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
#ifndef INCLUDE_DB_RETRY_DB_HPP_
#define INCLUDE_DB_RETRY_DB_HPP_

#include "common.hpp"
#include "app_config.hpp"
#include "events/events.hpp"
#include "events/event_system.hpp"
#include "types/repertory.hpp"
#include "utils/path_utils.hpp"
#include "utils/rocksdb_utils.hpp"

namespace repertory {
class retry_db final {
public:
  typedef std::function<bool(const std::string &api_path)> process_callback;

public:
  explicit retry_db(const app_config &config);

  ~retry_db();

private:
  std::unique_ptr<rocksdb::DB> db_;
  bool paused_ = false;
  std::mutex processing_mutex_;
  const std::string ROCKS_DB_NAME = "retry_db";

public:
  bool exists(const std::string &api_path) const;

  void pause();

  bool process_all(const process_callback &process);

  void remove(const std::string &api_path);

  void rename(const std::string &from_api_path, const std::string &to_api_path);

  void resume();

  void set(const std::string &api_path);
};
} // namespace repertory

#endif // INCLUDE_DB_RETRY_DB_HPP_
