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

#define FWD(v) std::forward<decltype(v)>(v)

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

    template<class T>
    struct scope_guard {
    private:
        T func;

    public:
        scope_guard(T&& func) : func(std::forward<T>(func)) {}
        ~scope_guard() {
            func();
        }
    };

    constexpr auto reverse(type_list<>) {
        return type_list<>{};
    }

    template<class T, class... Ts>
    constexpr auto reverse(type_list<T, Ts...>) {
        using reversed = decltype(reverse(type_list<Ts...>{}))::template push<T>;
        return reversed{};
    }

    template<int N>
    struct fixed_str {
        constexpr fixed_str(const char(&str)[N]) {
            for (int i = 0; i < N; ++i) {
                data[i] = str[i];
            }
        }

        constexpr auto view() const {
            return std::string_view(data);
        }

        char data[N] = {};
    };

    template<class T, class... Ts>
    constexpr auto first(type_list<T, Ts...> list) {
        return type_wrapper<T>{};
    }

    template<class T, class... Ts>
    constexpr auto pop_first(type_list<T, Ts...> list) {
        return type_list<Ts...>{};
    }

    template<class T, class... Ts>
    constexpr auto push_first(type_wrapper<T>, type_list<Ts...>) {
        return type_list<T, Ts...>{};
    }

    template<class... Ts, class... Us>
    constexpr auto filter(type_list<Ts...> list, auto predicate, type_list<Us...> acc = type_list{}) {
        if constexpr (sizeof...(Ts) == 0) {
            return acc;
        }
        else {
            constexpr auto rest = pop_first(list);
            constexpr auto head = first(list);
            if constexpr (!predicate(head)) {
                return filter(rest, predicate, acc);
            }
            else {
                return filter(rest, predicate, type_list<Us..., decltype(head.get())>{});
            }
        }
    }

    template<auto... vs>
    struct value_list {
        template<auto V>
        using push_back = value_list<vs..., V>;

        using types = type_list<decltype(vs)...>;

        static constexpr auto size = sizeof...(vs);
    };

    template<class T, class... Ts>
    constexpr auto contains(type_wrapper<T>, type_list<Ts...>) {
        return (... || std::is_same_v<T, Ts>);
    }

    template <class... Ts, class... Us>
    constexpr auto unique(type_list<Ts...> input, type_list<Us...> output = type_list{}) {
        if constexpr (sizeof...(Ts) == 0) {
            return output;
        }
        else {
            auto type = first(input);
            auto rest = pop_first(input);
            if constexpr (contains(type, output)) {
                return unique(rest, output);
            }
            else {
                return unique(rest, push_first(type, output));
            }
        }
    }

    template<class T, class... Ts>
    constexpr const T* try_get(const std::variant<Ts...>& v) {
        if constexpr (contains(type_wrapper<T>{}, type_list<Ts...>{})) {
            return std::get_if<T>(&v);
        }
        else {
            return nullptr;
        }
    }

    template <typename... Args>
    constexpr bool all(Args... args) {
      return (... && args);
    }

    template <auto... Args>
    inline constexpr bool all_v = (... && Args);

    TEMPLATE_CONCEPT(type_wrapper)
 }