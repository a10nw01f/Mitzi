#pragma once

#include "../utils.h"
#include "../profile.h"
#include "control_flow_parser.h"
#include "../validation_error.h"
#include <utility>


namespace mitzi {
	enum class borrow_command {
		ref,
		mut
	};

	struct borrow_record {
		borrow_command command;
		int id;
	};

	template<class T, class record, int N>
	class borrowable {
	public:
		template<class... Ts>
		borrowable(type_wrapper<T>, record_and_id<record, N>, Ts&&... args) :
			value(std::forward<Ts>(args)...)
		{}

		template<
			auto eval = [] {},
			auto v = record::template add < borrow_record{ borrow_command::ref, N }, eval > ()
		>
		const T& ref() const {
			return value;
		}

		template<
			auto eval = [] {},
			auto v = record::template add < borrow_record{ borrow_command::mut, N }, eval > ()
		>
		T& mut() {
			return value;
		}

	private:
		T value;
	};

	struct borrow_state {
		int ref_count = 0;
		int mut_count = 0;
	};

	struct borrow_rule {
		template<class state>
		struct mixin {
			template<
				class T,
				auto eval = [] {},
				auto id = state::counter::template next<eval>(),
				class... Ts
			>
			static auto borrow(Ts&&... args) {
				return borrowable(
					type_wrapper<std::remove_cvref_t<T>>{},
					record_and_id<typename state::record, id>{},
					std::forward<Ts>(args)...);
			}

			template<
				auto eval = [] {},
				auto id = state::counter::template next<eval>(),
				class T
			>
			static auto borrow(T&& arg) {
				return borrowable(
					type_wrapper<std::remove_cvref_t<T>>{},
					record_and_id<typename state::record, id>{},
					std::forward<T>(arg));
			}
		};

		template<class... Ts, std::size_t extent>
		static consteval std::optional<validation_error<borrow_state>> validate(std::span<const std::variant<Ts...>, extent> records) {
			std::vector<std::vector<borrow_state>> borrows = { {} };


			for (int record_index = 0; record_index < records.size(); record_index++) {
				if (auto flow = try_get<control_flow>(records[record_index]); flow) {
					if (is_end_scope(*flow)) {
						borrows.pop_back();
					}
					if (is_start_scope(*flow)) {
						auto copy = borrows.back();
						borrows.emplace_back(std::move(copy));
					}
				}
				else if (auto record = try_get<borrow_record>(records[record_index]); record) {
					if (borrows.back().size() <= record->id) {
						borrows.back().resize(record->id + 1);
					}
					auto& state = borrows.back()[record->id];
					switch (record->command)
					{
					case borrow_command::ref:
						state.ref_count++;
						break;
					case borrow_command::mut:
						state.mut_count++;
						break;
					default:
						std::unreachable();
					}

					if (state.mut_count > 1 ||
						(state.mut_count == 1 && state.ref_count > 0)) {
						return validation_error(record_index, state);
					}
				}
			}

			return std::nullopt;
		}
	};
}
