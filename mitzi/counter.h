#pragma once

#include "meta_state.h"
#include "utils.h"

namespace mitzi {
	template<auto Eval = [] {} >
	struct counter {
		using state = meta_state<value_wrapper<0>, Eval>;

		template<
			auto eval = [] {},
			auto current = state::template get<eval>::value,
			class v = state::template set<value_wrapper<current + 1>, eval>
		>
		static constexpr auto next() {
			return current;
		}
	};
}