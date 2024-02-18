#pragma once

#include "meta_state.h"
#include "utils.h"
#include "defer.h"
#include <utility>

namespace mitzi {

    struct destroyed {};

    template<
        class T,
        auto teval = [] {}
    >
    class handle {
    private:
        using state = meta_state<none, teval>;

        template<class... Ts>
        handle(Ts&&... args) :
            value(std::forward<Ts>(args)...) {
        }

    public:
        template<
            class defer,
            auto eval = [] {},
            class v = MITZI_DEFER_PUSH(defer, defer_assert<state, destroyed>),
            class... Ts
        >
        static auto make(defer, Ts&&... args) {
            return handle(std::forward<Ts>(args)...);
        }

        template<
            auto eval = [] {},
            class prev = state::template get<eval>,
            class v = state::template set<destroyed, std::pair(eval, teval)>,
            class... Ts
        >
        void destroy(Ts&&... args) {
            static_assert(!std::is_same_v<prev, destroyed>, "destroy was called twice");
            value.destroy(std::forward<Ts>(args)...);
        }

        template<class... Ts>
        void raw_destroy(Ts&&... args) {
            value.destroy(std::forward<Ts>(args)...);
        }

        template<
            auto eval = [] {},
            class prev = state::template get<eval >
        >
        auto& get() {
            static_assert(!std::is_same_v<prev, destroyed>, "already destroyed");
            return value;
        }

        template<
            auto eval = [] {},
            class defer,
            class v = MITZI_DEFER_PUSH(defer, defer_set<state, destroyed>),
            class v1 = MITZI_DEFER_PUSH(defer, defer_assert<state, none>),
            class... Ts
        >
        auto make_scope_guard(defer, Ts&&... args) {
            return scope_guard([&, this] {
                this->raw_destroy(std::forward<Ts>(args)...);
            });
        }

    private:
        T value;
    };

    template<
        auto eval = [] {},
        class defer_scope,
        class T
    >
    constexpr auto make_handle(defer_scope, const T& value) {
        using handle_t = handle<T, eval>;
        return handle_t::template make<defer_scope, eval>(defer_scope{}, value);
    }

}