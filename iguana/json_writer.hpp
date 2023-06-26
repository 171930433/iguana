//
// Created by Qiyu on 17-6-5.
//

#ifndef SERIALIZE_JSON_HPP
#define SERIALIZE_JSON_HPP
#include "define.h"
#include "detail/charconv.h"
#include "json_util.hpp"

namespace iguana {

template <typename Stream, sequence_container_t T>
IGUANA_INLINE void render_json_value(Stream &ss, const T &v);

template <typename Stream, typename InputIt, typename T, typename F>
IGUANA_INLINE void join(Stream &ss, InputIt first, InputIt last, const T &delim,
                        const F &f) {
  if (first == last)
    return;

  f(*first++);
  while (first != last) {
    ss.push_back(delim);
    f(*first++);
  }
}

template <typename Stream>
IGUANA_INLINE void render_json_value(Stream &ss, std::nullptr_t) {
  ss.append("null");
}

template <typename Stream>
IGUANA_INLINE void render_json_value(Stream &ss, bool b) {
  ss.append(b ? "true" : "false");
};

template <typename Stream>
IGUANA_INLINE void render_json_value(Stream &ss, char value) {
  ss.append("\"");
  ss.push_back(value);
  ss.append("\"");
}

template <typename Stream, num_t T>
IGUANA_INLINE void render_json_value(Stream &ss, T value) {
  char temp[65];
  auto p = detail::to_chars(temp, value);
  ss.append(temp, p - temp);
}

template <typename Stream, string_container_t T>
IGUANA_INLINE void render_json_value(Stream &ss, T &&t) {
  ss.push_back('"');
  if constexpr (fixed_array<T>) {
    constexpr auto n = sizeof(T) / sizeof(decltype(std::declval<T>()[0]));
    ss.append(std::begin(t), n);
  } else {
    ss.append(t.data(), t.size());
  }
  ss.push_back('"');
}

template <typename Stream, arithmetic_t T>
IGUANA_INLINE void render_key(Stream &ss, T &t) {
  ss.push_back('"');
  render_json_value(ss, t);
  ss.push_back('"');
}

template <typename Stream, string_container_t T>
IGUANA_INLINE void render_key(Stream &ss, T &&t) {
  render_json_value(ss, std::forward<T>(t));
}

template <typename Stream, refletable T> void to_json(T &&t, Stream &ss);

template <typename Stream, refletable T>
IGUANA_INLINE void render_json_value(Stream &ss, T &&t) {
  to_json(std::forward<T>(t), ss);
}

template <typename Stream, enum_t T>
IGUANA_INLINE void render_json_value(Stream &ss, T val) {
  render_json_value(ss, static_cast<std::underlying_type_t<T>>(val));
}

template <typename Stream, typename T>
IGUANA_INLINE void render_json_value(Stream &ss, std::optional<T> &val) {
  if (!val) {
    render_json_value(ss, std::string("null"));
  } else {
    render_json_value(ss, *val);
  }
}

template <typename Stream, typename T>
IGUANA_INLINE void render_array(Stream &ss, const T &v) {
  ss.push_back('[');
  join(ss, std::begin(v), std::end(v), ',',
       [&ss](const auto &jsv)
           IGUANA__INLINE_LAMBDA { render_json_value(ss, jsv); });
  ss.push_back(']');
}

template <typename Stream, typename T, size_t N>
IGUANA_INLINE void render_json_value(Stream &ss, const T (&v)[N]) {
  render_array(ss, v);
}

template <typename Stream, typename T, size_t N>
IGUANA_INLINE void render_json_value(Stream &ss, const std::array<T, N> &v) {
  render_array(ss, v);
}

template <typename Stream, associat_container_t T>
IGUANA_INLINE void render_json_value(Stream &ss, const T &o) {
  ss.push_back('{');
  join(ss, o.cbegin(), o.cend(), ',',
       [&ss](const auto &jsv) IGUANA__INLINE_LAMBDA {
         render_key(ss, jsv.first);
         ss.push_back(':');
         render_json_value(ss, jsv.second);
       });
  ss.push_back('}');
}

template <typename Stream, sequence_container_t T>
IGUANA_INLINE void render_json_value(Stream &ss, const T &v) {
  ss.push_back('[');
  join(ss, v.cbegin(), v.cend(), ',',
       [&ss](const auto &jsv)
           IGUANA__INLINE_LAMBDA { render_json_value(ss, jsv); });
  ss.push_back(']');
}

constexpr auto write_json_key = [](auto &s, auto i,
                                   auto &t) IGUANA__INLINE_LAMBDA {
  s.push_back('"');
  constexpr auto name =
      get_name<decltype(t),
               decltype(i)::value>(); // will be replaced by string_view later
  s.append(name.data(), name.size());
  s.push_back('"');
};

template <typename Stream, sequence_container_t T>
IGUANA_INLINE void to_json(T &&v, Stream &s) {
  using U = typename std::decay_t<T>::value_type;
  s.push_back('[');
  const size_t size = v.size();
  for (size_t i = 0; i < size; i++) {
    if constexpr (is_reflection_v<U>) {
      to_json(v[i], s);
    } else {
      render_json_value(s, v[i]);
    }

    if (i != size - 1)
      s.push_back(',');
  }
  s.push_back(']');
}

template <typename Stream, associat_container_t T>
IGUANA_INLINE void to_json(T &&t, Stream &s) {
  render_json_value(s, std::forward<T>(t));
}

template <typename Stream, tuple_t T>
IGUANA_INLINE void to_json(T &&t, Stream &s) {
  using U = typename std::decay_t<T>;
  s.push_back('[');
  const size_t size = std::tuple_size_v<U>;
  for_each(std::forward<T>(t),
           [&s, size](auto &v, auto i) IGUANA__INLINE_LAMBDA {
             render_json_value(s, v);

             if (i != size - 1)
               s.push_back(',');
           });
  s.push_back(']');
}

template <typename Stream, tuple_t T>
IGUANA_INLINE void render_json_value(Stream &ss, const T &v) {
  to_json(v, ss);
}

template <typename Stream, refletable T>
IGUANA_INLINE void to_json(T &&t, Stream &s) {
  s.push_back('{');
  for_each(std::forward<T>(t),
           [&t, &s](const auto &v, auto i) IGUANA__INLINE_LAMBDA {
             using M = decltype(iguana_reflect_members(std::forward<T>(t)));
             constexpr auto Idx = decltype(i)::value;
             constexpr auto Count = M::value();
             static_assert(Idx < Count);

             write_json_key(s, i, t);
             s.push_back(':');

             if constexpr (!is_reflection<decltype(v)>::value) {
               render_json_value(s, t.*v);
             } else {
               to_json(t.*v, s);
             }

             if (Idx < Count - 1)
               s.push_back(',');
           });
  s.push_back('}');
}

} // namespace iguana
#endif // SERIALIZE_JSON_HPP
