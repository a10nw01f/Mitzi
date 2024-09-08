#include "mitzi/mitzi.h"

#include "mitzi/profile.h"
#include "mitzi/rules/control_flow_parser.h"
#include "mitzi/rules/ptr_rule.h"
#include "mitzi/rules/borrow_rule.h"

#include <vector>

using namespace mitzi;

int main() {
	using safe_profile = profile<control_flow_parser_rule, ptr_rule, borrow_rule>;
	run(safe_profile{}, []<class M>(M, auto get_records) {
		auto vec = M::borrow(std::vector{ 42 });

		// uncommenting any of the line will produce an error
		M_({ )
			auto outer = M::ptr(vec.ref()[0]);
			M_({)
				auto vec2 = M::borrow(std::vector{ 0xCA7 });
				auto inner = M::ptr(vec2.ref()[0]);
				inner.assign(outer);
				//vec.mut().clear();
				//vec2.mut().clear();
				//outer.assign(inner);
			M_(})
		M_(})

		auto& mut1 = vec.mut();
		//auto& mut2 = vec.mut();
		//auto& ref1 = vec.ref();
	});

	return 0;
}