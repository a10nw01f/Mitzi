#pragma once

#include <utility>
#include "meta_state.h"
#include "utils.h"
#include "defer.h"

namespace mitzi {

    struct borrow_state {
        int read_count = 0;
        int write_count = 0;

        constexpr auto read() const {
            auto self = *this;
            self.read_count++;
            return self;
        }

        constexpr auto write() const {
            auto self = *this;
            self.write_count++;
            return self;
        }
    };

    template<class T, auto teval = [] {} >
    class borrowable {
        using state = meta_state < value_wrapper < borrow_state{} > , teval > ;

        template<auto eval>
        using end_write = decltype(defer_modify(state{}, [](auto type) {
            return type.modify([](auto value) {
                value.write_count--;
                return value;
                });
            }, value_wrapper<eval>{}));

        template<auto eval>
        using end_read = decltype(defer_modify(state{}, [](auto type) {
            return type.modify([](auto value) {
                value.read_count--;
                return value;
                });
            }, value_wrapper<eval>{}));
    public:
        template<class... Ts>
        explicit borrowable(Ts&&... args) : value(std::forward<Ts>(args)...) {}

        explicit borrowable(const T& value) : value(value) {}

        template<
            auto eval = [] {},
            class defer,
            auto next_state = decltype(state{}.template type<eval>()){}.get().write(),
            class v = state::template set<value_wrapper<next_state>, std::pair(eval, teval)>,
            class action = end_write<eval>,
            class v1 = MITZI_DEFER_PUSH(defer, action) >
        auto & mut(defer) {
            static_assert(next_state.read_count == 0 && next_state.write_count == 1, "");
            return value;
        }

        template<
            auto eval = [] {},
            class defer,
            auto next_state = decltype(state{}.template type<std::pair(eval, teval)>()){}.get().read(),
            class v = state::template set<value_wrapper<next_state>, std::pair(eval, teval)>,
            class action = end_read<eval>,
            class v1 = MITZI_DEFER_PUSH(defer, action) >
        auto & ref(defer) const {
            static_assert(next_state.write_count == 0, "");
            return value;
        }

    private:
        T value;
    };

}
