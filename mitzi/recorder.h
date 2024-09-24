#pragma once

#include "utils.h"
#include "validation_error.h"
#include "meta_state.h"
#include <variant>
#include <array>

namespace mitzi {
	template<auto... vs>
	constexpr auto unpack_records(value_list<vs...>) {
		using types = type_list<decltype(vs)...>;
		using element_type = decltype(unique(types{}))::template rename<std::variant>;

		return std::array<element_type, sizeof...(vs)>{ element_type(vs)... };
	}

	template<validation_error error, auto Eval = [] {} >
	struct recorder {
		using state = meta_state < value_list < none{} > , Eval > ;

		template<
			auto value,
			auto eval = [] {}
		>
		static constexpr auto add() {
			using next = state::template get<eval>::template push_back<value>;
			using v = state::template set<next, eval>;

			if constexpr (next::size - 1 == error.record_index) {
				assert_error<error.message>();
			}

			return true;
		}

		template<
			auto eval = [] {},
			class list = state::template get<eval>
		>
		static constexpr auto get() {
			return unpack_records(list{});
		}
	};

	template<
		class record,
		auto eval = [] {},
		class list = record::state:: template get<eval>
	>
	using records_getter = decltype([] {
		return unpack_records(list{});
		});

	template<class Record, int ID>
	struct record_and_id {
		static constexpr auto id = ID;
		using record = Record;
	};
}
