#include <iostream>

#include "mitzi/mitzhi.h"

using namespace mitzi;

struct device {};
struct texture {
    explicit texture(device) {
        std::cout << "creating texture\n";
    }

    void destroy(device) {
        std::cout << "destroying texture\n";
    }
};
struct texture_view {
    explicit texture_view(device, texture) {
        std::cout << "creating texture view\n";
    }
    void destroy(device, texture) {
        std::cout << "destroying texture view\n";
    }
};

auto handle_test() {
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
    return 0;
};

int handle_test_result = handle_test();

auto scope_guard_test() {
    using defer_scope = defer<>;
    auto my_device = device{};

    auto my_texture = make_handle(defer_scope{}, texture(my_device));
    auto texture_scope_guard = my_texture.make_scope_guard<[]{}>(defer_scope{}, my_device);

    auto my_texture_view = make_handle(defer_scope{}, texture_view(my_device, my_texture.get<[]{}>()));
    auto texture_view_scope_guard = my_texture_view.make_scope_guard<[]{}>(defer_scope{}, my_device, my_texture.get<[]{}>());

    defer_scope{}.apply<[]{}>();
    return 0;
}

int scope_guard_test_result = scope_guard_test();

auto lifetime_test() {
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
    return 0;
}

int lifetime_test_result = lifetime_test();

auto borrow_test() {
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
    return 0;
}

int borrow_test_result = borrow_test();

int main() {
	return 0;
}
