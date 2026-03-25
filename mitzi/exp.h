#pragma once

#include <utility>
#include "utils.h"
#include "ir.h"
#include "recorder_stack.h"

namespace mitzi {
struct default_exp_base {};

template <class T>
struct base_provider {
  using base = default_exp_base;
};

template <class Fn, class... Ts>
using exp_type = decltype(Fn{}(std::declval<Ts>()()...));

template <class Fn, class T>
struct exp_value {
  constexpr exp_value(auto&&... args) : value(Fn{}(FWD(args())...)) {}
  T value;

   T operator()() { return value; }
};

template <class Fn>
struct exp_value<Fn, void> {
  constexpr exp_value(auto&&... args) { Fn{}(FWD(args())...); }
  void operator()() {}
};

template <ir::func_name name,
          typename Fn,
          auto eval = [] {},
          class R = recorder_stack::get<eval>,
          typename... Args>
class exp
    : public base_provider<std::remove_cvref_t<exp_type<Fn, Args...>>>::base,
      private exp_value<Fn, exp_type<Fn, Args...>> {
 public:
  static constexpr auto ID = R::template id_to_index<eval>();

private:
  using type = exp_type<Fn, Args...>;
  static constexpr auto TYPE_ID =
      R::template id_to_index<type_wrapper<type>{}>();


  static constexpr auto EXPR =
      ir::exp(ID, 
              TYPE_ID,
              name,
              std::array<int, sizeof...(Args)>{std::decay_t<Args>::ID...});

  static_assert(R::add(value_wrapper<EXPR>{}), "");

  using type_exp_value = exp_value<Fn, type>;
public:
  using base = typename base_provider<std::remove_cvref_t<type>>::base;
  friend base;

  exp(value_wrapper<name>, Fn, Args&&... a)
      : type_exp_value(std::forward<Args>(a)...) {}

  exp(const exp& other) = default;
  exp(exp&& other) = default;
  exp& operator=(exp&& other) = default;
  exp& operator=(const exp& other) = default;

  auto operator=(this auto&& self, auto&& other)
    requires (std::remove_cvref_t<decltype(other)>::ID != ID) {
    return FWD(self).assignment_operator(FWD(other));
  }

  auto operator()() -> decltype(auto) { return type_exp_value::operator()(); }
};

template <class Name,
          typename Fn,
          auto eval = [] {},
          class R = recorder_stack::get<eval>,
          typename... Args>
exp(Name, Fn, Args&&...) -> exp<Name{}.get(), Fn, eval, R, Args...>;

TEMPLATE_CONCEPT(exp)

template <class T,
          auto eval = [] {},
          class R = recorder_stack::get<eval>,
          int... IDS>
class var {
 private:
  static constexpr auto ID = R::template id_to_index<eval>();
  static constexpr auto TYPE_ID = R::template id_to_index<type_wrapper<T>{}>();
  static constexpr auto EXPR = ir::exp(ID,
                                       TYPE_ID,
                                       ir::func_name("@constructor"),
                                       std::array<int, sizeof...(IDS)>{IDS...});
  static_assert(R::add(value_wrapper<EXPR>()), "");

  struct mut_helper {
    T& value;
    static constexpr auto ID = var::ID;
    T& operator()() const { return value; }
  };

  struct ref_helper {
    const T& value;
    static constexpr auto ID = var::ID;
    const T& operator()() const { return value; }
  };

  T value;

 public:
  var(auto&& arg)
    requires std::is_constructible_v<T, decltype(arg)>
      : value(arg) {}

  var(auto&&... args)
    requires std::is_constructible_v<T, decltype(args)...>
      : value(FWD(args)...) {}

  template<class U>
  var(std::initializer_list<U> list)
    requires std::is_constructible_v<T, decltype(list)>
      : value(FWD(list)) {}

  var(exp_c auto&& arg)
    requires std::is_constructible_v<T, decltype(arg())>
      : value(arg()) {}

  var(exp_c auto&&... args) 
    requires std::is_constructible_v<T, decltype(args())...>
      : value(args()...) {}

  T& operator()() { return value; }

  template <auto eeval = [] {}>
  auto mut() {
    return exp(
        value_wrapper<ir::func_name(".mut")>{},
        [](auto&& value) -> decltype(auto) { return value; },
        mut_helper{value});
  }

  template <auto eeval = [] {}>
  auto ref() const {
    return exp(
        value_wrapper<ir::func_name(".ref")>{},
        [](auto&& value) -> decltype(auto) { return value; },
        ref_helper{value});
  }
};

template <auto eval = [] {}, class R = recorder_stack::get<eval>>
var(auto&& arg) -> var<std::remove_cvref_t<decltype(arg)>, eval, R>;

template <auto eval = [] {}, class R = recorder_stack::get<eval>>
var(exp_c auto&& arg) -> var<std::remove_cvref_t<decltype(arg())>,
                             eval,
                             R,
                             std::decay_t<decltype(arg)>::ID>;

template <auto eval = [] {}, class R = recorder_stack::get<eval>>
auto nop() {
  return exp(value_wrapper<ir::func_name("nop")>{}, [] {});
}

template <auto v, auto eval = [] {}, class R = recorder_stack::get<eval>>
auto constant() {
  return exp(value_wrapper<ir::func_name("constant")>{}, [] { return v; });
}

}
