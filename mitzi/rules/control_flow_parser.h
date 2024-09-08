#pragma once

#include "../recorder.h"
#include "../utils.h"
#include <optional>
#include <variant>
#include <string_view>
#include <span>

namespace mitzi {
	enum class control_flow {
		start_scope,
		end_scope,
		if_,
		else_if,
		else_,
		for_loop,
		while_loop,
		return_,
		break_
	};

	constexpr auto is_start_scope(control_flow flow) {
		return flow == control_flow::start_scope ||
			flow == control_flow::if_ ||
			flow == control_flow::else_ ||
			flow == control_flow::else_if ||
			flow == control_flow::for_loop ||
			flow == control_flow::while_loop;
	}

	constexpr auto is_end_scope(control_flow flow) {
		return flow == control_flow::end_scope ||
			flow == control_flow::else_ ||
			flow == control_flow::else_if;
	}

	struct control_flow_parser_rule {
		template<class state>
		struct mixin {
			static consteval control_flow parse_control(std::string_view str) {
				if (str == "{") return control_flow::start_scope;
				if (str == "}") return control_flow::end_scope;
				if (str.starts_with("if (") && str.ends_with(") {")) return control_flow::if_;
				if (str.starts_with("for (") && str.ends_with(") {")) return control_flow::for_loop;
				if (str.starts_with("for (") && str.ends_with(") {")) return control_flow::while_loop;
				if (str == "return;" || (str.starts_with("return ") && str.ends_with(";"))) return control_flow::return_;
				if (str == "break;") return control_flow::break_;

				std::unreachable();
			}

			template<
				fixed_str str,
				auto eval = [] {},
				auto control = parse_control(str.view()),
				auto v = state::record::template add<control, eval>()
			>
			using parse = decltype(control);
		};

		template<class... Ts, std::size_t extent>
		static consteval std::optional<validation_error<none>> validate(std::span<const std::variant<Ts...>, extent> records) {
			return std::nullopt;
		}
	};
}

#define M_(...) sizeof(typename M::template parse<#__VA_ARGS__>); __VA_ARGS__


