#pragma once

#include <utility>

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
        friend auto inject_state_func(reader<N, tag>) {
            return T{};
        }

        static constexpr state_t<N, T> state{};
    };

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
            return state_t<N - 1, decltype(inject_state_func(r))>{};
        }
    }


    template<
        auto tag,
        auto eval = [] {},
        auto State = get_last_state<tag, eval>()
    >
    using get_state = typename std::remove_cvref_t<decltype(State)>;


    template<
        typename T,
        auto tag,
        auto eval
    >
    consteval auto set_state_impl() {
        using cur_state = decltype(get_last_state<tag, eval>());
        setter<cur_state::n + 1, T, tag> s;
        return s.state;
    }

    template<
        class T,
        auto tag,
        auto eval = [] {},
        auto state = set_state_impl<T, tag, eval>()
    >
    constexpr auto set_state = [] { return state; };

    struct none {};

    template<class Init = none, auto tag = [] {} >
    class meta_state {
    public:
        template<
            typename T,
            auto eval = [] {}
        >
        using set = decltype(set_state<T, tag, eval>());

        template<auto eval = [] {} >
        using get = typename get_state<tag, eval>::type;

        template<auto eval = [] {} >
        constexpr auto type() const->meta_state::get<eval>;
    private:
        static constexpr setter<0, Init, tag> init = {};
    };

}
