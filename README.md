# Mitzi - Advanced Compile-Time Validation for C++

Mitzi is a header-only library designed to provide advanced compile-time validation capabilities in C++. Leveraging the power of stateful metaprogramming, Mitzi aims to enhance code safety and performance by offering features such as borrow checking, lifetime management, and destructor arguments. It uses C++23 and is currently in its early stages of development.

## Features

### Borrow Checking
Mitzi provides borrow checking mechanisms to ensure safe access to shared resources. It restricts having either a single mutable refernce of multiple immutable ones.

```cpp
using defer_scope = defer<>;
auto value = borrowable(42);

{
	using defer_scope = defer<>;

	auto& ref1 = value.ref<[]{}>(defer_scope{});
	auto& ref2 = value.ref<[]{}>(defer_scope{});
	auto& ref3 = value.ref<[]{}>(defer_scope{});

	// auto& mut1 = value.mut<[]{}>(defer_scope{});
	// uncommenting the line above will cause a static assert

	defer_scope{}.apply<[]{}>();
}

{
	using defer_scope = defer<>;

	auto& mut1 = value.mut<[]{}>(defer_scope{});

	// auto& ref1 = value.ref(defer_scope{});
	// auto& mut2 = value.mut(defer_scope{});
	// uncommenting any of the lines above will cause a static assert

	defer_scope{}.apply<[]{}>();
}

defer_scope{}.apply<[]{}>();
```

### Lifetimes Management
One of Mitzi's core functionalities is managing object lifetimes to prevent dangling pointers. By employing compile-time checks, it ensures that pointers do not refer to destroyed objects, thus mitigating the risk of accessing invalid memory locations.

```cpp
constexpr auto lifetimes = lifetime_factory<>{};
using defer_scope = decltype(lifetimes.begin_scope<[]{}>());

int outer_value = 42;
ptr outer_ptr(outer_value, lifetimes.add_lifetime<[]{}>());
{
	using defer_scope = decltype(lifetimes.begin_scope<[]{}>());

	int inner_value = 3;
	ptr inner_ptr(inner_value, lifetimes.add_lifetime<[]{}>());

	inner_ptr = outer_ptr;

	// outer_ptr = inner_ptr;
	// uncommenting the above line will cause a static_assert

	defer_scope{}.apply<[]{}>();
}

defer_scope{}.apply<[]{}>();
```

### Destructor Arguments
Pass arguments to a cleanup function directly or via a scope guard. It validates that the cleanup function is called once for every object, preventing potential memory leaks and ensuring proper cleanup.

```cpp
using defer_scope = defer<>;

auto my_device = device{};
auto my_texture = make_handle(defer_scope{}, texture(my_device));
auto my_texture_view = make_handle(defer_scope{}, texture_view(my_device, my_texture.get<[]{}>()));

my_texture_view.destroy<[]{}>(my_device, my_texture.get<[]{}>());
my_texture.destroy<[]{}>(my_device);
/*
	removing or changing the order of the destroy calls will cause a static assert
*/

defer_scope{}.apply<[]{}>();
```

### Custom Validation
Use the exsiting infrastructure to create new validation rules for your unique requirements.

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

## Dedicated to
This library is dedicated to my dearly beloved cat Mitzi who recently passed away.
I only wish to spend more time with you in this lifetime of the next.
