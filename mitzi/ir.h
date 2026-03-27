#pragma once

#include <array>
#include <span>
#include <variant>
#include "utils.h"
#include "fixed_str.h"
#include "meta_state.h"

namespace mitzi::ir {

using func_name = fixed_string<32>;

struct exp {
  int id;
  int type;
  func_name fn_name;
  std::array<int, 8> args = {};
  int arg_count;

  template <auto N>
  constexpr exp(int id, int type, func_name fn, const std::array<int, N>& args)
      : id(id), type(type), fn_name(fn), arg_count(N) {
    for (int i = 0; i < N; i++) {
      this->args[i] = args[i];
    }
  }

  constexpr auto get_args() const { 
    return std::span(args.data(), arg_count);
  }
};

struct start {};

enum class control_flow { start, end, _if, else_if, _else, _for, _return };

#define scope_ \
  {            \
    mitzi::recorder_stack::get<[] {}>::add( \
        mitzi::value_wrapper<mitzi::ir::control_flow::start>{});
#define end_                             \
  mitzi::recorder_stack::get<[] {}>::add(          \
      mitzi::value_wrapper<mitzi::ir::control_flow::end>{}); \
  }
#define if_                              \
  mitzi::recorder_stack::get<[] {}>::add(          \
      mitzi::value_wrapper<mitzi::ir::control_flow::_if>{}); \
  if
#define return_                              \
  mitzi::recorder_stack::get<[] {}>::add(       \
      mitzi::value_wrapper<mitzi::ir::control_flow::_return>{}); \
  return 
#define for_                                                  \
  mitzi::recorder_stack::get<[] {}>::add(                        \
      mitzi::value_wrapper<mitzi::ir::control_flow::_for>{}); \
  for
#define else_if_                                                \
  mitzi::recorder_stack::get<[] {}>::add(                       \
    mitzi::value_wrapper<mitzi::ir::control_flow::end>{});      \
  mitzi::recorder_stack::get<[] {}>::add(                       \
      mitzi::value_wrapper<mitzi::ir::control_flow::else_if>{});\
  } else if


#define else_                                                \
  mitzi::recorder_stack::get<[] {}>::add(                        \
      mitzi::value_wrapper<mitzi::ir::control_flow::end>{});     \
  mitzi::recorder_stack::get<[] {}>::add(                        \
      mitzi::value_wrapper<mitzi::ir::control_flow::_else>{}); \
  }                                                              \
  else 

using instruction = std::variant<start, exp, control_flow>;

template<class Ids, class Records> 
struct fn_info {
  using IDs = Ids;
  constexpr auto get_instructions() const { 
    return[]<class... Ts>(type_list<Ts...>) {
      return std::array<instruction, sizeof...(Ts)>{instruction(Ts{}.get())...};
    }(Records{});
  }

  constexpr auto get_ids() const { return Ids{}; }
};

template <auto eeval = [] {}>
struct recorder {
  using instructions = meta_state<value_wrapper<start{}>>;
  using ids = meta_state<value_wrapper<start{}>>;

  template <auto id,
            class T = ids::template set<value_wrapper<id>, id>,
            auto index = ids::template get_version<value_wrapper<id>{}>>
  static constexpr auto id_to_index() {
    return static_cast<int>(index);
  }

  template <auto inst, auto eval = [] {}>
    requires(std::is_same_v<decltype(inst), exp> ||
             std::is_same_v<decltype(inst), control_flow>)
  static consteval auto add(value_wrapper<inst>) {
    using T = instructions::template set<value_wrapper<inst>>;
    return !std::is_same_v<T, decltype(eval)>;
  }

  template <auto eval = [] {},
            class T1 = ids::template all_versions<eval>,
            class T2 = instructions::template all_versions<eval>>
  static constexpr auto get_ir() {
    using R = fn_info<T1, T2>;
    return R{};
  }
};

TEMPLATE_CONCEPT(recorder)
}