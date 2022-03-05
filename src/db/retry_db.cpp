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
#include "db/retry_db.hpp"

namespace repertory {
retry_db::retry_db(const app_config &config) {
  utils::db::create_rocksdb(config, ROCKS_DB_NAME, db_);
}

retry_db::~retry_db() { db_.reset(); }

bool retry_db::exists(const std::string &api_path) const {
  std::string value;
  return db_->Get(rocksdb::ReadOptions(), api_path, &value).ok();
}

void retry_db::pause() {
  mutex_lock processing_lock(processing_mutex_);
  paused_ = true;
}

bool retry_db::process_all(const process_callback &process) {
  auto processed = false;
  if (not paused_) {
    unique_mutex_lock processing_lock(processing_mutex_);
    if (not paused_) {
      auto iterator = std::unique_ptr<rocksdb::Iterator>(db_->NewIterator(rocksdb::ReadOptions()));
      for (iterator->SeekToFirst(); not paused_ && iterator->Valid(); iterator->Next()) {
        const auto api_path = iterator->value().ToString();
        if (process(api_path)) {
          remove(api_path);
          processed = true;
        }

        processing_lock.unlock();
        std::this_thread::sleep_for(1ms);
        processing_lock.lock();
      }
    }
    processing_lock.unlock();
  }

  return processed;
}

void retry_db::remove(const std::string &api_path) {
  db_->Delete(rocksdb::WriteOptions(), api_path);
}

void retry_db::rename(const std::string &from_api_path, const std::string &to_api_path) {
  if (exists(from_api_path)) {
    remove(from_api_path);
    set(to_api_path);
  }
}

void retry_db::resume() {
  mutex_lock processing_lock(processing_mutex_);
  paused_ = false;
}

void retry_db::set(const std::string &api_path) {
  db_->Put(rocksdb::WriteOptions(), api_path, api_path);
}
} // namespace repertory
