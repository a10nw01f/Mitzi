#pragma once

#include <utility>
#include "utils.h"

namespace mitzi {

    template<int N, class T>
    struct state_t {
        static constexpr int n = N;
        using type = T;
    };


    template<
        int N,
        auto tag
    >
    struct reader {
        friend auto inject_state_func(reader<N, tag>);
    };

    template<
        int N,
        class T,
        auto tag
    >
    struct setter {
        static constexpr state_t<N, T> state{};

        friend auto inject_state_func(reader<N, tag>) {
            return state;
        }
    };

    template <auto tag, auto version, auto eval = []{}>
    using get_state_type = typename decltype(inject_state_func(reader<version, tag>{}))::type;

    template<
        auto tag,
        auto eval,
        int N = 0
    >
    consteval auto get_last_state() {
        constexpr bool counted_past_n = requires(reader<N, tag> r) {
            inject_state_func(r);
        };

        if constexpr (counted_past_n) {
            return get_last_state<tag, eval, N + 1>();
        }
        else {
            constexpr reader<N - 1, tag> r;
            return decltype(inject_state_func(r)){};
        }
    }


    template<
        auto tag,
        auto eval = [] {},
        auto State = get_last_state<tag, eval>()
    >
    using get_state = typename std::remove_cvref_t<decltype(State)>;


    template<
        class T,
        auto tag,
        auto eval
    >
    consteval auto set_state_impl() {
        using cur_state = decltype(get_last_state<tag, eval>());
        using next = setter<cur_state::n + 1, T, tag>;
        return next{}.state;
    }

    template <auto tag,
              auto eval = [] {},
              auto size = decltype(get_last_state<tag, eval>())::n,
              auto... Is>
    consteval auto get_all_versions(std::index_sequence<Is...> = {}) {
      if constexpr (sizeof...(Is) == size + 1) {
        return type_list<get_state_type<tag, Is>...>{};
      } else {
        return get_all_versions<tag, eval, size>(
            std::make_index_sequence<size + 1>{});
      }
    }

    struct none {};

    template<class Init = none, auto ttag = [] {} >
    class meta_state {
    public:
        template<
            typename T,
            auto eval = [] {}
        >
        using set = decltype(set_state_impl<T, ttag, eval>());

        template<
            auto eval = [] {}
        >
        static constexpr auto get_version = get_state<ttag, eval>::n;

        template<auto eval = [] {} >
        using get = typename get_state<ttag, eval>::type;

        static constexpr auto tag = ttag;

        template <auto eval = [] {}>
        using all_versions = decltype(get_all_versions<ttag, eval>()); 
    private: 
      static constexpr setter<0, Init, ttag> init = {};
    };

}
