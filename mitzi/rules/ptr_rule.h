#pragma once

#include "../profile.h"
#include "../validation_error.h"
#include "control_flow_parser.h"
#include <vector>

namespace mitzi {
	enum class ptr_command {
		init,
		assign
	};

	struct ptr_record {
		ptr_command command;
		int self_id;
		int other_id;
	};

	template<
		class T,
		class record,
		int id
	>
	class ptr {
		template<
			class TT,
			class recordT,
			int idT
		> friend class ptr;
	public:
		ptr(T& value, record_and_id<record, id>) : data(&value) {}

		template<
			auto eval = [] {},
			int other_id = -1,
			auto v = record::template add < ptr_record{ ptr_command::assign, id, other_id }, eval > ()
		>
		ptr& assign(ptr<T, record, other_id> other) {
			this->data = other.data;
			return *this;
		}

	private:
		T* data;
	};

	struct error_lifetime_is_not_long_enough {};

	struct ptr_rule {
		template<class state>
		struct mixin {
			template<
				auto eval = [] {},
				auto id = state::counter::template next<eval>(),
				class T
			>
			static constexpr auto ptr(T&& value) {
				constexpr auto v = state::record::template add < ptr_record{ ptr_command::init, id, -1 } > ();
				return mitzi::ptr(value, mitzi::record_and_id<typename state::record, id>{});
			}
		};

		template<class... Ts, std::size_t extent>
		static constexpr std::optional<validation_error<error_lifetime_is_not_long_enough>>
			validate(std::span<const std::variant<Ts...>, extent> records) {
			std::vector<int> ptrs;
			int depth = 1;

			for (int record_index = 0; record_index < records.size(); record_index++) {
				if (auto flow = try_get<control_flow>(records[record_index]); flow) {
					if (is_end_scope(*flow)) {
						depth--;
					}
					if (is_start_scope(*flow)) {
						depth++;
					}
				}
				else if (const ptr_record* rcd = try_get<ptr_record>(records[record_index]); rcd) {
					switch (rcd->command)
					{
					case ptr_command::init:
						if (ptrs.size() <= rcd->self_id) {
							ptrs.resize(rcd->self_id + 1, 0);
						}
						ptrs[rcd->self_id] = depth;
						break;
					case ptr_command::assign:
						if (ptrs[rcd->self_id] < ptrs[rcd->other_id]) {
							return validation_error(record_index, error_lifetime_is_not_long_enough{});
						}
						break;
					}
				}
			}

			return std::nullopt;
		}
	};
}
