# Mitzi - Advanced Compile-Time Validation for C++

Mitzi is a header-only library designed to provide advanced compile-time validation capabilities in C++. Leveraging the power of stateful metaprogramming, Mitzi aims to enhance code safety and performance by offering features such as borrow checking, lifetime management, and destructor arguments. It uses C++23 and is currently in its early stages of development.

### Compiler Explorer - https://godbolt.org/z/hfz77hc4K

### Blog post of the initial implementation (outdated) - https://a10nw01f.github.io/post/advanced_compile_time_validation/

## How to use

### Create Custom Validation Rules

Each rule contains two parts:  
Mixin - the interface of the rule, it encapsulates the meta state and records actions.  
Validate - function that detects an error based on the recorded actions.  

```cpp
struct my_rule {
	struct custom_error {};
	struct custom_action {};

	template<class state>
	struct mixin {
		template<
			auto eval = []{},
			auto v - state::recorder::template add<custom_action>()>()
		>
		void foo() { }
	};

	static constexpr std::optional<mitzi::validation_error<custom_error>> validate(auto actions) {
		/*...*/
	}
};
```

### Profiles

Create a profile that validates a set of rules:

```cpp
using safe_profile = profile<control_flow_parser_rule, ptr_rule, borrow_rule>;
```

Validate a lambda with a profile:
```cpp
mitzi::run(safe_profile{}, []<class M>(M, auto get_actions) {
	constexpr auto actions = get_actions();
	// actions == std::array{ init, borrow_action(ref, 0), borrow_action(mut, 0) };
	
	auto num = M::borrow(42);
	auto& ref1 = num.ref();
	auto& mut1 = num.mut();
})
```

`M` is the combined interface of all the validation rules in the profile.  
`get_actions` returns all the actions that the lambda records.  


## Usage
```cpp
#include <mitzi/mitzi.h>
```

## Contribution
Mitzi is an ongoing project, and contributions are welcome! Whether it's through reporting bugs, suggesting new features, or submitting pull requests, your involvement helps make Mitzi better for everyone.

## License
Mitzi is licensed under the MIT License. You are free to use, modify, and distribute this software according to the terms of the license.

## Disclaimer
Mitzi is still in the early stages of development. While it aims to provide robust compile-time validation features, it may contain bugs or limitations. Please use it with caution and feel free to report any issues you encounter.

## Dedication
This library is dedicated to my dearly beloved cat Mitzi who recently passed away.
I only wish to spend more time with you in this lifetime of the next.
