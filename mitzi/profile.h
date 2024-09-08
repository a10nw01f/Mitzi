#pragma once

#include "recorder.h"
#include "counter.h"
#include "meta_state.h"
#include <optional>
#include <span>

namespace mitzi {
	template<validation_error error, auto Eval = [] {} >
	struct profile_state {
		using counter = mitzi::counter < [] {} > ;
		using record = mitzi::recorder < error, [] {} > ;
	};

	template<class T, class U>
	using get_mixin = T::template mixin<U>;

	constexpr auto default_validation_error = validation_error<none>{ -1, none{} };

	template<class... Ts>
	struct profile {
		template<auto Eval, validation_error error_index = default_validation_error >
		struct validator : get_mixin<Ts, profile_state<error_index, Eval>>...{
			using state = profile_state<error_index, Eval>;
			using rules = type_list<Ts...>;

			template<
				auto eval = [] {},
				class list = state::record::state:: template get<eval>
			>
			static constexpr auto validate(decltype(eval) = {}) {
				constexpr auto records = unpack_records(list{});

				return [records](this auto&& self, auto rules) {
					if constexpr (rules.size() == 0) {
						return std::optional<none>(std::nullopt);
					}
					else {
						constexpr auto error = decltype(first(rules).get())::validate(std::span(records));
						if constexpr (error) {
							return error;
						}
						else {
							return self(pop_first(rules));
						}
					}
					}(rules{});
			}

			template<
				auto eval = [] {},
				class list = state::record::state:: template get<eval>
			>
			static constexpr auto get_records(decltype(eval) = {}) {
				return unpack_records(list{});
			}
		};
	};

	template<
		class Profile,
		class Fn,
		auto eval = [] {},
		class init = Profile::template validator<eval, default_validation_error >,
		class init_records = records_getter<typename init::state::record>,
		class Ret = decltype(std::declval<Fn>()(init{}, init_records{})),
		class records = records_getter<typename init::state::record>
	>
	decltype(auto) run(Profile, Fn&& fn, decltype(eval) = {}) {
		static constexpr auto optional_error = init::validate();
		static constexpr auto error = [] {
			if constexpr (optional_error) {
				return *optional_error;
			}
			else {
				return default_validation_error;
			}
			}();

		using new_state = Profile::template validator < [] {}, error > ;
		return fn(new_state{}, records{});
		if constexpr (optional_error) {
			assert_error<optional_error->message>();
		}
	}
}