#pragma once

namespace mitzi {
	template<class T>
	struct validation_error {
		int record_index = -1;
		T message;
	};

	template<auto V>
	inline constexpr auto assert_error_impl = false;

	template<auto v>
	constexpr auto assert_error() {
		static_assert(assert_error_impl<v>);
	}
}