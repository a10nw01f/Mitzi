#pragma once

#include "utils.h"
#include "meta_state.h"
#include "ir.h"

namespace mitzi {
struct recorder_stack {
  using stack = meta_state<type_list<>>;

  template <auto eval = [] {},
            class next = stack::template get<>::template push_front<ir::template recorder<>>,
            class TT = stack::set<next>>
  static consteval auto push() {}

  template <auto eval = [] {},
            class next = decltype(pop_front(stack::get<eval>{})),
            class TT = stack::set<next>>
  static consteval auto pop() {}

  template <auto eval = []{}>
  using get = decltype(mitzi::first(stack::get<eval>{}).get());

  template <auto eval = [] {},
            class T = decltype(mitzi::first(stack::get<eval>{}).get())>
  static consteval auto get_ir() {
    return T::get_ir();
  }
};
}