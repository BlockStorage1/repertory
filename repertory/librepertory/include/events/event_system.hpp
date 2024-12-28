/*
  Copyright <2018-2024> <scott.e.graves@protonmail.com>

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
#ifndef REPERTORY_INCLUDE_EVENTS_EVENT_SYSTEM_HPP_
#define REPERTORY_INCLUDE_EVENTS_EVENT_SYSTEM_HPP_

#include "events/event.hpp"
#include "events/t_event_system.hpp"

namespace repertory {
using event_system = t_event_system<event>;
using event_consumer = event_system::event_consumer;

#define E_FROM_API_FILE_ERROR(e) api_error_to_string(e)
#define E_FROM_BOOL(t) std::to_string(t)
#define E_FROM_CURL_CODE(t) std::string(curl_easy_strerror(t))
#define E_FROM_DOUBLE(d) std::to_string(d)
#define E_FROM_DOUBLE_PRECISE(dbl_val)                                         \
  ([](const double &d) -> std::string {                                        \
    std::stringstream ss;                                                      \
    ss << std::fixed << std::setprecision(2) << d;                             \
    return ss.str();                                                           \
  })(dbl_val)
#define E_FROM_INT32(t) std::to_string(t)
#define E_FROM_SIZE_T(t) std::to_string(t)
#define E_FROM_STRING(t) t
#define E_FROM_UINT16(t) std::to_string(t)
#define E_FROM_UINT64(t) std::to_string(t)
#define E_FROM_DOWNLOAD_TYPE(t) download_type_to_string(t)

#define E_PROP(type, name, short_name, ts)                                     \
private:                                                                       \
  void init_##short_name(const type &val) {                                    \
    auto ts_val = ts(val);                                                     \
    ss_ << "|" << #short_name << "|" << ts_val;                                \
    j_[#name] = ts_val;                                                        \
  }                                                                            \
                                                                               \
public:                                                                        \
  [[nodiscard]] auto get_##name() const->json { return j_[#name]; }

#define E_BEGIN(name, el)                                                      \
  class name final : public virtual event {                                    \
  private:                                                                     \
    name(const std::stringstream &ss, const json &j, bool allow_async)         \
        : event(ss, j, allow_async) {}                                         \
                                                                               \
  public:                                                                      \
    ~name() override = default;                                                \
                                                                               \
  public:                                                                      \
    static const event_level level = event_level::el;                          \
                                                                               \
  public:                                                                      \
    [[nodiscard]] auto get_name() const -> std::string override {              \
      return #name;                                                            \
    }                                                                          \
                                                                               \
    [[nodiscard]] auto get_event_level() const -> event_level override {       \
      return name::level;                                                      \
    }                                                                          \
                                                                               \
    [[nodiscard]] auto get_single_line() const -> std::string override {       \
      const auto s = ss_.str();                                                \
      return get_name() + (s.empty() ? "" : s);                                \
    }                                                                          \
                                                                               \
    [[nodiscard]] auto clone() const -> std::shared_ptr<event> override {      \
      return std::shared_ptr<name>(new name(ss_, j_, get_allow_async()));      \
    }
#define E_END() }

#define E_SIMPLE(event_name, el, allow_async)                                  \
  E_BEGIN(event_name, el)                                                      \
public:                                                                        \
  event_name() : event(allow_async) {}                                         \
  E_END()

#define E_SIMPLE1(event_name, el, allow_async, type, name, short_name, tc)     \
  E_BEGIN(event_name, el)                                                      \
  explicit event_name(const type &tv) : event(allow_async) {                   \
    init_##short_name(tv);                                                     \
  }                                                                            \
  E_PROP(type, name, short_name, tc)                                           \
  E_END()

#define E_SIMPLE2(event_name, el, allow_async, type, name, short_name, tc,     \
                  type2, name2, short_name2, tc2)                              \
  E_BEGIN(event_name, el)                                                      \
  explicit event_name(const type &tv, const type2 &tv2) : event(allow_async) { \
    init_##short_name(tv);                                                     \
    init_##short_name2(tv2);                                                   \
  }                                                                            \
  E_PROP(type, name, short_name, tc)                                           \
  E_PROP(type2, name2, short_name2, tc2)                                       \
  E_END()

#define E_SIMPLE3(event_name, el, allow_async, type, name, short_name, tc,     \
                  type2, name2, short_name2, tc2, type3, name3, short_name3,   \
                  tc3)                                                         \
  E_BEGIN(event_name, el)                                                      \
  explicit event_name(const type &tv, const type2 &tv2, const type3 &tv3)      \
      : event(allow_async) {                                                   \
    init_##short_name(tv);                                                     \
    init_##short_name2(tv2);                                                   \
    init_##short_name3(tv3);                                                   \
  }                                                                            \
  E_PROP(type, name, short_name, tc)                                           \
  E_PROP(type2, name2, short_name2, tc2)                                       \
  E_PROP(type3, name3, short_name3, tc3)                                       \
  E_END()

#define E_SIMPLE4(event_name, el, allow_async, type, name, short_name, tc,     \
                  type2, name2, short_name2, tc2, type3, name3, short_name3,   \
                  tc3, type4, name4, short_name4, tc4)                         \
  E_BEGIN(event_name, el)                                                      \
  explicit event_name(const type &tv, const type2 &tv2, const type3 &tv3,      \
                      const type4 &tv4)                                        \
      : event(allow_async) {                                                   \
    init_##short_name(tv);                                                     \
    init_##short_name2(tv2);                                                   \
    init_##short_name3(tv3);                                                   \
    init_##short_name4(tv4);                                                   \
  }                                                                            \
  E_PROP(type, name, short_name, tc)                                           \
  E_PROP(type2, name2, short_name2, tc2)                                       \
  E_PROP(type3, name3, short_name3, tc3)                                       \
  E_PROP(type4, name4, short_name4, tc4)                                       \
  E_END()

#define E_SIMPLE5(event_name, el, allow_async, type, name, short_name, tc,     \
                  type2, name2, short_name2, tc2, type3, name3, short_name3,   \
                  tc3, type4, name4, short_name4, tc4, type5, name5,           \
                  short_name5, tc5)                                            \
  E_BEGIN(event_name, el)                                                      \
  explicit event_name(const type &tv, const type2 &tv2, const type3 &tv3,      \
                      const type4 &tv4, const type5 &tv5)                      \
      : event(allow_async) {                                                   \
    init_##short_name(tv);                                                     \
    init_##short_name2(tv2);                                                   \
    init_##short_name3(tv3);                                                   \
    init_##short_name4(tv4);                                                   \
    init_##short_name5(tv5);                                                   \
  }                                                                            \
  E_PROP(type, name, short_name, tc)                                           \
  E_PROP(type2, name2, short_name2, tc2)                                       \
  E_PROP(type3, name3, short_name3, tc3)                                       \
  E_PROP(type4, name4, short_name4, tc4)                                       \
  E_PROP(type5, name5, short_name5, tc5)                                       \
  E_END()

#define E_SIMPLE6(event_name, el, allow_async, type, name, short_name, tc,     \
                  type2, name2, short_name2, tc2, type3, name3, short_name3,   \
                  tc3, type4, name4, short_name4, tc4, type5, name5,           \
                  short_name5, tc5, type6, name6, short_name6, tc6)            \
  E_BEGIN(event_name, el)                                                      \
  explicit event_name(const type &tv, const type2 &tv2, const type3 &tv3,      \
                      const type4 &tv4, const type5 &tv5, const type6 &tv6)    \
      : event(allow_async) {                                                   \
    init_##short_name(tv);                                                     \
    init_##short_name2(tv2);                                                   \
    init_##short_name3(tv3);                                                   \
    init_##short_name4(tv4);                                                   \
    init_##short_name5(tv5);                                                   \
    init_##short_name6(tv6);                                                   \
  }                                                                            \
  E_PROP(type, name, short_name, tc)                                           \
  E_PROP(type2, name2, short_name2, tc2)                                       \
  E_PROP(type3, name3, short_name3, tc3)                                       \
  E_PROP(type4, name4, short_name4, tc4)                                       \
  E_PROP(type5, name5, short_name5, tc5)                                       \
  E_PROP(type6, name6, short_name6, tc6)                                       \
  E_END()

#define E_SIMPLE7(event_name, el, allow_async, type, name, short_name, tc,     \
                  type2, name2, short_name2, tc2, type3, name3, short_name3,   \
                  tc3, type4, name4, short_name4, tc4, type5, name5,           \
                  short_name5, tc5, type6, name6, short_name6, tc6, type7,     \
                  name7, short_name7, tc7)                                     \
  E_BEGIN(event_name, el)                                                      \
  explicit event_name(const type &tv, const type2 &tv2, const type3 &tv3,      \
                      const type4 &tv4, const type5 &tv5, const type6 &tv6,    \
                      const type7 &tv7)                                        \
      : event(allow_async) {                                                   \
    init_##short_name(tv);                                                     \
    init_##short_name2(tv2);                                                   \
    init_##short_name3(tv3);                                                   \
    init_##short_name4(tv4);                                                   \
    init_##short_name5(tv5);                                                   \
    init_##short_name6(tv6);                                                   \
    init_##short_name7(tv7);                                                   \
  }                                                                            \
  E_PROP(type, name, short_name, tc)                                           \
  E_PROP(type2, name2, short_name2, tc2)                                       \
  E_PROP(type3, name3, short_name3, tc3)                                       \
  E_PROP(type4, name4, short_name4, tc4)                                       \
  E_PROP(type5, name5, short_name5, tc5)                                       \
  E_PROP(type6, name6, short_name6, tc6)                                       \
  E_PROP(type7, name7, short_name7, tc7)                                       \
  E_END()

#define E_CONSUMER()                                                           \
private:                                                                       \
  std::vector<std::shared_ptr<repertory::event_consumer>> event_consumers_

#define E_CONSUMER_RELEASE() event_consumers_.clear()

#define E_SUBSCRIBE(name, callback)                                            \
  event_consumers_.emplace_back(std::make_shared<repertory::event_consumer>(   \
      #name, [this](const event &evt) { callback(evt); }))

#define E_SUBSCRIBE_EXACT(name, callback)                                      \
  event_consumers_.emplace_back(std::make_shared<repertory::event_consumer>(   \
      #name, [this](const event &evt) {                                        \
        callback(dynamic_cast<const name &>(evt));                             \
      }))

#define E_SUBSCRIBE_ALL(callback)                                              \
  event_consumers_.emplace_back(std::make_shared<repertory::event_consumer>(   \
      [this](const event &evt) { callback(evt); }))
} // namespace repertory

#endif // REPERTORY_INCLUDE_EVENTS_EVENT_SYSTEM_HPP_
