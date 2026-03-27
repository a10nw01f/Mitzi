#pragma once

#include <utility>
#include <string_view>
#include <variant>
#include <tuple>

#define TEMPLATE_CONCEPT_EX(name, tname)    \
  template <class T>                        \
  concept name##_c = requires(T arg) { \
    { tname{arg} } -> std::same_as<std::remove_cvref_t<T>>;      \
  };
#define TEMPLATE_CONCEPT(name) TEMPLATE_CONCEPT_EX(name, name)

#define FWD(...) std::forward<decltype(__VA_ARGS__)>(__VA_ARGS__)

namespace mitzi {

    template<class... Ts>
    struct type_list {
        template<class T>
        using push = type_list<Ts..., T>;

        template<class T>
        using push_front = type_list<T, Ts...>;

        template<template<class...> class T>
        using rename = T<Ts...>;

        constexpr auto size() const { return sizeof...(Ts); }
    };

    template<class T, class... Ts>
    consteval auto pop_front(type_list<T, Ts...>) {
      return type_list<Ts...>{};
    }

    template<class T>
    struct type_wrapper {
        constexpr T get() const;
    };

    template<auto V>
    struct value_wrapper {
        constexpr auto get() const { return V; }

        template<class F>
        constexpr auto modify(F) const { return value_wrapper < F{}(V) > (); }

        static constexpr auto value = V;
    };

    template<class F, class... Ts>
    inline constexpr void for_each(F&& func, Ts&&... args) {
        (func(args), ...);
    }

    template <class F, auto... Is, class... Ts>
    inline constexpr void for_each_indexed(F&& func, std::index_sequence<Is...> = {}, Ts && ... args) {
      if constexpr (sizeof...(Is) == sizeof...(Ts)) {
        (func(args, value_wrapper<Is>{}), ...);
      } else {
        for_each_indexed(FWD(func), std::index_sequence_for<Ts...>{},
                         FWD(args)...);
      }
    }

    template <auto size, class F, auto... Is>
    inline constexpr void for_each_index(F&& func, value_wrapper<size> vw, std::index_sequence<Is...> = {}) {
      if constexpr (sizeof...(Is) == size) {
        (func(value_wrapper<Is>{}), ...);
      } else {
        for_each_index(FWD(func), vw, std::make_index_sequence<size>());
      }
    }

    template <auto size, class F, auto... Is>
    inline constexpr auto expand_indices(F&& func,
                                         value_wrapper<size> vw,
                                         std::index_sequence<Is...> = {}) {
      if constexpr (sizeof...(Is) == size) {
        return func(value_wrapper<Is>{}...);
      } else {
        expand_indices(FWD(func), vw, std::make_index_sequence<size>());
      }
    }

    template<class F, class... Ts>
    inline constexpr void for_each_type(F&& func, type_list<Ts...>) {
        for_each(func, type_wrapper<Ts>{}...);
    }

    template <class F, class... Ts>
    inline constexpr void for_each_type_indexed(F&& func, type_list<Ts...>) {
      for_each_indexed(func, std::index_sequence_for<Ts...>{}, type_wrapper<Ts>{}...);
    }

    template <class F, class... Ts>
    inline constexpr void for_each_type_indexed_unwrap(F&& func, type_list<Ts...>) {
      for_each_indexed(func, std::index_sequence_for<Ts...>{}, Ts{}...);
    }

    template<class T, class... Ts>
    constexpr auto first(type_list<T, Ts...> list) {
        return type_wrapper<T>{};
    }

    TEMPLATE_CONCEPT(type_wrapper)
 }