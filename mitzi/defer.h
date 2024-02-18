#pragma once

#include "meta_state.h"
#include "utils.h"

namespace mitzi {

	template<auto teval = [] {} >
	struct defer {
		using state = meta_state<type_list<>, teval>;

		template<
			auto eval = [] {},
			class F,
			class v = typename state::template set<typename state::template get<eval>::template push_front<F>, eval>
		>
		constexpr auto push(F arg) const {
			return 0;
		}

		template<
			auto tteval = [] {} >
		constexpr auto apply() const {
			using List = typename state::template get<tteval>;
			constexpr auto value = [] {
#ifdef __clang__
				auto list = reverse(List{});
#else
				auto list = List{};
#endif

				for_each_type([]<class T>(type_wrapper<T>) {
					constexpr auto eval = [] {};
					T{}.template apply<std::pair(eval, teval)>();
				}, list);
				return 0;
			}();

			return value;
		}
	};

	template<class state, class V>
	struct defer_set {
		template<
			auto eval = [] {},
			class v = state::template set<V, eval>
		>
		constexpr auto apply() {
			return v{};
		}
	};

	template<class state, class T>
	struct defer_assert {
		template<
			auto eval = [] {} >
		constexpr auto apply() {
			static_assert(std::is_same_v<typename state::template get<eval>, T>, "");
		}
	};

	template<class state, class F, auto teval = [] {} >
	struct defer_modify {
		template<
			auto eval = [] {},
			class prev_state = decltype(state{}.template type<eval>()),
			class next_state = decltype(std::declval<F>()(std::declval<prev_state>())),
			class v = state::template set<next_state, eval>
		>
		constexpr auto apply() const {}

		constexpr defer_modify() = default;

		constexpr defer_modify(state, F, value_wrapper<teval> arg = {}) {}
	};

}

#define MITZI_DEFER_PUSH(defer, ...) typename defer::state::template set<\
            typename defer::state::template get<>::template push_front<\
                __VA_ARGS__>>
