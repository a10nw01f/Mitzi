#pragma once

#include "meta_state.h"
#include "utils.h"

namespace mitzi {

	struct lifetime {
		int counter = 0;
		int depth = 0;

		constexpr bool outlives(const lifetime other) const {
			if (depth < other.depth) {
				return true;
			}
			else if (other.depth < depth) {
				return false;
			}

			return counter > other.counter;
		}

		constexpr auto inc() const {
			auto self = *this;
			self.counter++;
			return self;
		}

		constexpr auto push() const {
			auto self = *this;
			self.depth++;
			return self;
		}

		constexpr auto pop() const {
			auto self = *this;
			self.depth--;
			return self;
		}
	};

	template<auto teval = [] {} >
	class lifetime_factory {
		using state = meta_state < value_wrapper < lifetime{} > , teval > ;

		template<auto eval>
		using pop_depth = decltype(defer_modify(state{}, [](auto type) {
			return type.modify([](auto value) {
				value.depth--;
				return value;
				});
			}, value_wrapper<eval>{}));
	public:
		template<
			auto eval = [] {},
			auto next_lifetime = decltype(state{}.template type<eval>()){}.get().inc(),
			class v = state::template set<value_wrapper<next_lifetime>, std::pair(eval, teval)> >
		constexpr v::type add_lifetime() const {
			return {};
		}

		template<
			auto eval = [] {},
			class defer_scope = defer<eval>,
			auto next_lifetime = decltype(state{}.template type<eval>()){}.get().push(),
			class v = state::template set<value_wrapper<next_lifetime>, std::pair(eval, teval)>,
			class action = pop_depth<eval>,
			class v1 = MITZI_DEFER_PUSH(defer_scope, action) >
		constexpr auto begin_scope(defer_scope arg = {}) const {
			return defer_scope{};
		}
	};

	template<
		class T,
		auto lifetime
	>
	class ptr {
	public:
		ptr(T& value, value_wrapper<lifetime>) : m_value(&value) {}

		template<auto other_lifetime>
		ptr& operator=(ptr<T, other_lifetime> other) {
			static_assert(other_lifetime.outlives(lifetime), "life time is not long enough");
			m_value = other.get();
			return *this;
		}

		auto get() const { return m_value; }
	private:
		T* m_value;
	};

}