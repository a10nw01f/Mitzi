#pragma once

#include <cstdint>
#include <utility>

namespace mitzi {
	template<class T, auto teval, class Flags = uint32_t>
	class usage {
		using state = meta_state<value_wrapper<(Flags)0>, teval>;
		T value;
	public:
		template<class... Ts>
		constexpr usage(Ts&&... args) : value(std::forward<Ts>(args)...)
		{}

		template<
			auto V,
			auto eval = [] {},
			auto next_state = decltype(state{}.template type<eval>()){}.get() | (Flags)V,
			class U = state::template set<value_wrapper<next_state>, eval>
		>
		auto& use() {
			return value;
		}

		template<
			auto eval = [] {},
			class U = state::get<eval>>
			constexpr auto get_usage() const {
			return U{}.get();
		}
	};
}
