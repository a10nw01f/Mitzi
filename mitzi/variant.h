#pragma once

#include<tuple>
#include<utility>
#include "utils.h"

namespace mitzi {

struct variant_end {
  constexpr variant_end() = default;
  constexpr variant_end(variant_end, int) {}
};

template <class T, class... Ts>
struct variant_impl {
  static constexpr auto get_rest_type() {
    if constexpr (sizeof...(Ts) == 0) {
      return type_wrapper<variant_end>{};
    } else {
      return type_wrapper<variant_impl<Ts...>>{};
    }
  }

  constexpr auto get_index() const { return sizeof...(Ts); }

  static constexpr auto INDEX = sizeof...(Ts);

  using Rest = decltype(get_rest_type().get());

  union {
    T m_Value;
    Rest m_Rest;
  };

  constexpr variant_impl() = default;

  constexpr variant_impl(auto&& value, int& index)
    requires(std::is_same_v<T, std::remove_cvref_t<decltype(value)>>)
      : m_Value(FWD(value)) {
    index = sizeof...(Ts);
  }

  constexpr variant_impl(auto&& value, int& index) : m_Rest(FWD(value), index) {}

  constexpr decltype(auto) visit(this auto&& self, auto&& func, int index) {
    if (index == self.get_index()) {
      return FWD(func)(std::forward_like<decltype(self)>(self.m_Value));
    } else {
      if constexpr (!std::is_same_v<Rest, variant_end>) {
        return FWD(self).m_Rest.visit(FWD(func), index);
      }
    }
  }

  template <class U>
  constexpr auto* get_if(this auto&& self, int index) {
    if constexpr (std::is_same_v<T, U>) {
      if (index == std::decay_t<decltype(self)>::INDEX) {
        return &self.m_Value;
      } else {
        return static_cast<decltype(&self.m_Value)>(nullptr);
      }
    } else {
      return self.m_Rest.template get_if<U>(index);
    }
  }
};

template <class... Ts>
struct variant {
  using impl = variant_impl<Ts...>;
  int m_Index = -1;
  impl m_Impl;

  constexpr variant() : m_Impl(variant_end{}, m_Index) {}
  constexpr variant(auto&& value)
      : m_Index(-1), m_Impl(std::forward<decltype(value)>(value), m_Index) {}

  constexpr ~variant() = default;
  constexpr variant(const variant& other) = default;
  constexpr variant(variant&& other) noexcept = default;
  constexpr variant(variant& other)
      : variant(static_cast<const variant&>(other)) {}
  constexpr variant& operator=(const variant& other) = default;
  constexpr variant& operator=(variant&& other) noexcept = default;

  constexpr decltype(auto) visit(this auto&& self, auto&& func) {
    return FWD(self).m_Impl.visit(FWD(func), self.m_Index);
  }

  template <class T>
  constexpr auto* get_if(this auto&& self) {
    return self.m_Impl.template get_if<T>(self.m_Index);
  }
};

}  // namespace mitzi