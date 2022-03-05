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
#ifndef INCLUDE_EVENTS_EVENT_SYSTEM_HPP_
#define INCLUDE_EVENTS_EVENT_SYSTEM_HPP_

#include "common.hpp"
#include "events/event.hpp"
#include "events/t_event_system.hpp"
#include "utils/string_utils.hpp"

namespace repertory {
typedef t_event_system<event> event_system;
typedef event_system::event_consumer event_consumer;

#define E_CAST(t) ((std::string)t)
#define E_DOUBLE(d) utils::string::from_double(d)
#define E_DOUBLE_PRECISE(d)                                                                        \
  ([](const double &d) -> std::string {                                                            \
    std::stringstream ss;                                                                          \
    ss << std::fixed << std::setprecision(2) << d;                                                 \
    return ss.str();                                                                               \
  })(d)
#define E_FROM_BOOL(t) utils::string::from_bool(t)
#define E_FROM_EXCEPTION(e) std::string(e.what() ? e.what() : "")
#define E_FROM_INT32(t) utils::string::from_int32(t)
#define E_FROM_INT64(t) utils::string::from_int64(t)
#define E_FROM_UINT16(t) utils::string::from_uint16(t)
#define E_FROM_STRING_ARRAY(a)                                                                     \
  ([](const auto &array) -> std::string {                                                          \
    std::stringstream ret;                                                                         \
    for (const auto &item : array) {                                                               \
      ret << (std::string(item) + " ");                                                            \
    }                                                                                              \
    return std::string(ret).TrimRight();                                                           \
  })(a)

#define E_PERCENT(d)                                                                               \
  ([](const double &d) -> std::string {                                                            \
    std::stringstream ss;                                                                          \
    ss << std::fixed << std::setprecision(2) << d;                                                 \
    ss << "%";                                                                                     \
    return ss;                                                                                     \
  })(d)
#define E_STRING(t) t
#define E_FROM_UINT8(t) utils::string::from_uint8(t)
#define E_FROM_UINT32(t) utils::string::from_uint32(t)
#define E_FROM_UINT64(t) utils::string::from_uint64(t)
#define E_FROM_SIZE_T(t) std::to_string(t)
#define E_FROM_API_FILE_ERROR(e) api_error_to_string(e)

#define E_PROP(type, name, short_name, ts)                                                         \
private:                                                                                           \
  void init_##short_name(const type &tv) {                                                         \
    ss_ << "|" << #short_name << "|" << ts(tv);                                                    \
    j_[#name] = ts(tv);                                                                            \
  }                                                                                                \
                                                                                                   \
public:                                                                                            \
  json get_##name() const { return j_[#name]; }

#define E_BEGIN(name, el)                                                                          \
  class name final : public virtual event {                                                        \
  private:                                                                                         \
    name(const std::stringstream &ss, const json &j, const bool &allow_async)                      \
        : event(ss, j, allow_async) {}                                                             \
                                                                                                   \
  public:                                                                                          \
    ~name() override = default;                                                                    \
                                                                                                   \
  public:                                                                                          \
    static const event_level level = event_level::el;                                              \
                                                                                                   \
  public:                                                                                          \
    std::string get_name() const override { return #name; }                                        \
                                                                                                   \
    event_level get_event_level() const override { return name::level; }                           \
                                                                                                   \
    std::string get_single_line() const override {                                                 \
      const auto s = ss_.str();                                                                    \
      return get_name() + (s.empty() ? "" : s);                                                    \
    }                                                                                              \
                                                                                                   \
    std::shared_ptr<event> clone() const override {                                                \
      return std::shared_ptr<name>(new name(ss_, j_, get_allow_async()));                          \
    }
#define E_END() }

#define E_SIMPLE(event_name, el, allow_async)                                                      \
  E_BEGIN(event_name, el)                                                                          \
public:                                                                                            \
  event_name() : event(allow_async) {}                                                             \
  E_END()

#define E_SIMPLE1(event_name, el, allow_async, type, name, short_name, tc)                         \
  E_BEGIN(event_name, el)                                                                          \
  explicit event_name(const type &tv) : event(allow_async) { init_##short_name(tv); }              \
  E_PROP(type, name, short_name, tc)                                                               \
  E_END()

#define E_SIMPLE2(event_name, el, allow_async, type, name, short_name, tc, type2, name2,           \
                  short_name2, tc2)                                                                \
  E_BEGIN(event_name, el)                                                                          \
  explicit event_name(const type &tv, const type2 &tv2) : event(allow_async) {                     \
    init_##short_name(tv);                                                                         \
    init_##short_name2(tv2);                                                                       \
  }                                                                                                \
  E_PROP(type, name, short_name, tc)                                                               \
  E_PROP(type2, name2, short_name2, tc2)                                                           \
  E_END()

#define E_SIMPLE3(event_name, el, allow_async, type, name, short_name, tc, type2, name2,           \
                  short_name2, tc2, type3, name3, short_name3, tc3)                                \
  E_BEGIN(event_name, el)                                                                          \
  explicit event_name(const type &tv, const type2 &tv2, const type3 &tv3) : event(allow_async) {   \
    init_##short_name(tv);                                                                         \
    init_##short_name2(tv2);                                                                       \
    init_##short_name3(tv3);                                                                       \
  }                                                                                                \
  E_PROP(type, name, short_name, tc)                                                               \
  E_PROP(type2, name2, short_name2, tc2)                                                           \
  E_PROP(type3, name3, short_name3, tc3)                                                           \
  E_END()

#define E_SIMPLE4(event_name, el, allow_async, type, name, short_name, tc, type2, name2,           \
                  short_name2, tc2, type3, name3, short_name3, tc3, type4, name4, short_name4,     \
                  tc4)                                                                             \
  E_BEGIN(event_name, el)                                                                          \
  explicit event_name(const type &tv, const type2 &tv2, const type3 &tv3, const type4 &tv4)        \
      : event(allow_async) {                                                                       \
    init_##short_name(tv);                                                                         \
    init_##short_name2(tv2);                                                                       \
    init_##short_name3(tv3);                                                                       \
    init_##short_name4(tv4);                                                                       \
  }                                                                                                \
  E_PROP(type, name, short_name, tc)                                                               \
  E_PROP(type2, name2, short_name2, tc2)                                                           \
  E_PROP(type3, name3, short_name3, tc3)                                                           \
  E_PROP(type4, name4, short_name4, tc4)                                                           \
  E_END()

#define E_SIMPLE5(event_name, el, allow_async, type, name, short_name, tc, type2, name2,           \
                  short_name2, tc2, type3, name3, short_name3, tc3, type4, name4, short_name4,     \
                  tc4, type5, name5, short_name5, tc5)                                             \
  E_BEGIN(event_name, el)                                                                          \
  explicit event_name(const type &tv, const type2 &tv2, const type3 &tv3, const type4 &tv4,        \
                      const type5 &tv5)                                                            \
      : event(allow_async) {                                                                       \
    init_##short_name(tv);                                                                         \
    init_##short_name2(tv2);                                                                       \
    init_##short_name3(tv3);                                                                       \
    init_##short_name4(tv4);                                                                       \
    init_##short_name5(tv5);                                                                       \
  }                                                                                                \
  E_PROP(type, name, short_name, tc)                                                               \
  E_PROP(type2, name2, short_name2, tc2)                                                           \
  E_PROP(type3, name3, short_name3, tc3)                                                           \
  E_PROP(type4, name4, short_name4, tc4)                                                           \
  E_PROP(type5, name5, short_name5, tc5)                                                           \
  E_END()

#define E_SIMPLE6(event_name, el, allow_async, type, name, short_name, tc, type2, name2,           \
                  short_name2, tc2, type3, name3, short_name3, tc3, type4, name4, short_name4,     \
                  tc4, type5, name5, short_name5, tc5, type6, name6, short_name6, tc6)             \
  E_BEGIN(event_name, el)                                                                          \
  explicit event_name(const type &tv, const type2 &tv2, const type3 &tv3, const type4 &tv4,        \
                      const type5 &tv5, const type6 &tv6)                                          \
      : event(allow_async) {                                                                       \
    init_##short_name(tv);                                                                         \
    init_##short_name2(tv2);                                                                       \
    init_##short_name3(tv3);                                                                       \
    init_##short_name4(tv4);                                                                       \
    init_##short_name5(tv5);                                                                       \
    init_##short_name6(tv6);                                                                       \
  }                                                                                                \
  E_PROP(type, name, short_name, tc)                                                               \
  E_PROP(type2, name2, short_name2, tc2)                                                           \
  E_PROP(type3, name3, short_name3, tc3)                                                           \
  E_PROP(type4, name4, short_name4, tc4)                                                           \
  E_PROP(type5, name5, short_name5, tc5)                                                           \
  E_PROP(type6, name6, short_name6, tc6)                                                           \
  E_END()

#define E_SIMPLE7(event_name, el, allow_async, type, name, short_name, tc, type2, name2,           \
                  short_name2, tc2, type3, name3, short_name3, tc3, type4, name4, short_name4,     \
                  tc4, type5, name5, short_name5, tc5, type6, name6, short_name6, tc6, type7,      \
                  name7, short_name7, tc7)                                                         \
  E_BEGIN(event_name, el)                                                                          \
  explicit event_name(const type &tv, const type2 &tv2, const type3 &tv3, const type4 &tv4,        \
                      const type5 &tv5, const type6 &tv6, const type7 &tv7)                        \
      : event(allow_async) {                                                                       \
    init_##short_name(tv);                                                                         \
    init_##short_name2(tv2);                                                                       \
    init_##short_name3(tv3);                                                                       \
    init_##short_name4(tv4);                                                                       \
    init_##short_name5(tv5);                                                                       \
    init_##short_name6(tv6);                                                                       \
    init_##short_name7(tv7);                                                                       \
  }                                                                                                \
  E_PROP(type, name, short_name, tc)                                                               \
  E_PROP(type2, name2, short_name2, tc2)                                                           \
  E_PROP(type3, name3, short_name3, tc3)                                                           \
  E_PROP(type4, name4, short_name4, tc4)                                                           \
  E_PROP(type5, name5, short_name5, tc5)                                                           \
  E_PROP(type6, name6, short_name6, tc6)                                                           \
  E_PROP(type7, name7, short_name7, tc7)                                                           \
  E_END()

#define E_CONSUMER()                                                                               \
private:                                                                                           \
  std::vector<std::shared_ptr<repertory::event_consumer>> event_consumers_

#define E_CONSUMER_RELEASE() event_consumers_.clear()

#define E_SUBSCRIBE(name, callback)                                                                \
  event_consumers_.emplace_back(                                                                   \
      std::make_shared<repertory::event_consumer>(#name, [this](const event &e) { callback(e); }))

#define E_SUBSCRIBE_EXACT(name, callback)                                                          \
  event_consumers_.emplace_back(std::make_shared<repertory::event_consumer>(                       \
      #name, [this](const event &e) { callback(dynamic_cast<const name &>(e)); }))

#define E_SUBSCRIBE_ALL(callback)                                                                  \
  event_consumers_.emplace_back(                                                                   \
      std::make_shared<repertory::event_consumer>([this](const event &e) { callback(e); }))
} // namespace repertory

#endif // INCLUDE_EVENTS_EVENT_SYSTEM_HPP_
