#pragma once

#include <source_location>
#include <string_view>
#include "utils.h"

class REFLECT_STRUCT_HELPER {
 public:
  enum class ENUM { };
};

namespace mitzi {

template <class... Ts>
constexpr auto function_name() noexcept -> std::string_view {
  return std::source_location::current().function_name();
}

template <class T>
struct type_name_info {
  static constexpr auto name = function_name<int>();
  static constexpr auto begin = name.find("int");
  static constexpr auto end =
      name.substr(begin + std::size(std::string_view{"int"}));
};

template <class T>
  requires std::is_class_v<T>
struct type_name_info<T> {
  static constexpr auto name = function_name<::REFLECT_STRUCT_HELPER>();
  static constexpr auto begin = name.find("REFLECT_STRUCT_HELPER");
  static constexpr auto end =
      name.substr(begin + std::size(std::string_view{"REFLECT_STRUCT_HELPER"}));
};

template <class T>
  requires std::is_enum_v<T>
struct type_name_info<T> {
  static constexpr auto name = function_name<::REFLECT_STRUCT_HELPER::ENUM>();
  static constexpr auto begin = name.find("REFLECT_STRUCT_HELPER::ENUM");
  static constexpr auto end =
      name.substr(begin + std::size(std::string_view{"REFLECT_STRUCT_HELPER::ENUM"}));
};

inline constexpr const char* ws = " \t\n\r\f\v";

inline constexpr void rtrim(std::string& s, const char* t = ws) {
  s.erase(s.find_last_not_of(t) + 1);
}

inline constexpr void ltrim(std::string& s, const char* t = ws) {
  s.erase(0, s.find_first_not_of(t));
}

inline constexpr void trim(std::string& s, const char* t = ws) {
  rtrim(s, t);
  ltrim(s, t);
}

template <class T>
constexpr auto get_type_name(type_wrapper<T>) {
  using TT = type_name_info<T>;
  auto fn_name = function_name<T>();
  auto name = fn_name.substr(TT::begin, fn_name.find(TT::end) - TT::begin);

  auto result = std::string(name);
  trim(result);
  return result;
}

}  // namespace mitzi