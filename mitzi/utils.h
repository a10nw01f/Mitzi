#pragma once

#include <utility>

namespace mitzi {

    template<class... Ts>
    struct type_list {
        template<class T>
        using push = type_list<Ts..., T>;

        template<class T>
        using push_front = type_list<T, Ts...>;
    };

    template<class T>
    struct type_wrapper {
        constexpr T get() const;
    };

    template<auto V>
    struct value_wrapper {
        constexpr auto get() const { return V; }

        template<class F>
        constexpr auto modify(F) const { return value_wrapper < F{}(V) > (); }
    };

    template<class F, class... Ts>
    inline constexpr void for_each(F&& func, Ts&&... args) {
        (func(args), ...);
    }

    template<class F, class... Ts>
    inline constexpr void for_each_type(F func, type_list<Ts...>) {
        for_each(func, type_wrapper<Ts>{}...);
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

}