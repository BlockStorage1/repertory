/*
  Copyright <2018-2025> <scott.e.graves@protonmail.com>

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
#include "utils/file_thread_file.hpp"

namespace repertory::utils::file {
// auto thread_file::attach_file(native_handle handle,
//                               bool read_only) -> fs_file_t {}

auto thread_file::attach_file(fs_file_t file) -> fs_file_t {
  return fs_file_t{
      new thread_file(std::move(file)),
  };
}

auto thread_file::open_file(std::string_view path,
                            bool read_only) -> fs_file_t {
  return fs_file_t{
      new thread_file(file::open_file(path, read_only)),
  };
}

auto thread_file::open_or_create_file(std::string_view path,
                                      bool read_only) -> fs_file_t {
  return fs_file_t{
      new thread_file(file::open_or_create_file(path, read_only)),
  };
}

void thread_file::io_item::done(bool result) {
  unique_mutex_lock lock(*mtx);
  complete = true;
  success = result;
  notify->notify_all();
}

void thread_file::io_item::wait() const {
  if (complete) {
    return;
  }

  unique_mutex_lock lock(*mtx);
  while (not complete) {
    notify->wait(lock);
  }
  notify->notify_all();
}

thread_file::thread_file(std::string_view path) : file_(new file(path)) {}

thread_file::thread_file(std::wstring_view path)
    : file_(new file(utils::string::to_utf8(path))) {}

thread_file::thread_file(fs_file_t file) : file_(std::move(file)) {}

thread_file::~thread_file() {
  close();

  if (io_thread_) {
    io_thread_->join();
  }
}

void thread_file::close() {
  do_io([this]() -> bool {
    file_->close();
    stop_requested_ = true;
    return true;
  });
}

auto thread_file::copy_to(std::string_view new_path,
                          bool overwrite) const -> bool {
  return do_io([this, &new_path, &overwrite]() -> bool {
    return file_->copy_to(new_path, overwrite);
  });
}

auto thread_file::do_io(action_t action) const -> bool {
  unique_mutex_lock lock(*mtx_);
  if (stop_requested_) {
    return false;
  }

  if (not io_thread_) {
    io_thread_ = std::make_unique<std::thread>([this]() { thread_func(); });
  }

  auto item = std::make_shared<io_item>(action);
  actions_.emplace_back(item);
  notify_->notify_all();
  lock.unlock();

  item->wait();

  return item->success;
}

void thread_file::flush() const {
  do_io([this]() -> bool {
    file_->flush();
    return true;
  });
}

auto thread_file::move_to(std::string_view path) -> bool {
  return do_io([this, &path]() -> bool { return file_->move_to(path); });
}

auto thread_file::read(unsigned char *data, std::size_t to_read,
                       std::uint64_t offset, std::size_t *total_read) -> bool {
  return do_io([this, &data, &to_read, &offset, &total_read]() -> bool {
    return file_->read(data, to_read, offset, total_read);
  });
}

auto thread_file::remove() -> bool {
  return do_io([this]() -> bool { return file_->remove(); });
}

void thread_file::thread_func() const {
  unique_mutex_lock lock(*mtx_);
  notify_->notify_all();
  lock.unlock();

  const auto run_actions = [this, &lock]() {
    auto actions = actions_;
    actions_.clear();
    notify_->notify_all();
    lock.unlock();

    for (auto &&action : actions) {
      action->done(action->action());
    }
  };

  while (not stop_requested_) {
    lock.lock();
    if (stop_requested_) {
      lock.unlock();
      break;
    }

    while (not stop_requested_ && actions_.empty()) {
      notify_->wait(lock);
    }

    if (stop_requested_) {
      lock.unlock();
      break;
    }

    run_actions();
  }

  lock.lock();
  run_actions();
}

auto thread_file::truncate(std::size_t size) -> bool {
  return do_io([this, &size]() -> bool { return file_->truncate(size); });
}

auto thread_file::write(const unsigned char *data, std::size_t to_write,
                        std::size_t offset,
                        std::size_t *total_written) -> bool {
  return do_io([this, &data, &to_write, &offset, &total_written]() -> bool {
    return file_->write(data, to_write, offset, total_written);
  });
}

auto thread_file::size() const -> std::optional<std::uint64_t> {
  std::optional<std::uint64_t> size;
  do_io([this, &size]() -> bool {
    size = file_->size();
    return size.has_value();
  });
  return size;
}
} // namespace repertory::utils::file
