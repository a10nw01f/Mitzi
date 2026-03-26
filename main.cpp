#include <vector>
#include <iostream>
#include "mitzi/mitzi.h"
#include "mitzi/macros.h"
#include "mitzi/analyzers/lexical_borrow_checker.h"
#include "mitzi/analyzers/nll_borrow_checker.h"
#include "mitzi/analyzers/print.h"

TEMPLATE_CONCEPT_EX(vector, std::vector)

namespace mitzi {
template <>
struct base_provider<int> {
  struct base {
    WRAP_BINARY_OP(+)
    WRAP_BINARY_OP(-)
    WRAP_BINARY_OP(>)
    WRAP_BINARY_OP(<)
    WRAP_BINARY_OP(<<)
    WRAP_BINARY_OP(==)

    WRAP_UNARY_OP(-)
    WRAP_UNARY_OP(&)

    WRAP_ASSIGNEMNT_OP()
  };
};

template <>
struct base_provider<bool> {
  struct base {
    operator bool(this auto&& self) { return bool(self()); }
  };
};

template <class T>
struct base_provider<std::vector<T>> {
  struct base {
    WRAP_METHOD(push_back)
    WRAP_METHOD(clear)
    WRAP_BINARY_OP_EX([], FWD(a)[FWD(b)])
  };
};

template <class T>
struct base_provider<T*> {
  struct base {
    WRAP_ASSIGNEMNT_OP()
    WRAP_UNARY_OP(*)
  };
};

template <>
struct base_provider<std::ostream> {
  struct base {
    WRAP_BINARY_OP(<<)
  };
};

template <class R, class T1>
constexpr auto get_lifetime_constraints(
    value_wrapper<ir::func_name("operator&")>,
    type_list<R, T1>,
    const ir::exp& expression) {
  return std::vector<lifetime_constraint>{
      {.superset = expression.args[0], .subset = expression.id}};
}

template <class R, vector_c T1, class T2>
constexpr auto get_lifetime_constraints(
    value_wrapper<ir::func_name("operator[]")>,
    type_list<R, T1, T2>,
    const ir::exp& expression) {
  return std::vector<lifetime_constraint>{
      {.superset = expression.args[0], .subset = expression.id}};
}

template <class R, class T1>
constexpr auto get_lifetime_constraints(
    value_wrapper<ir::func_name("@constructor")>,
    type_list<R*, T1>,
    const ir::exp& expression) {
  return std::vector<lifetime_constraint>{
      {.superset = expression.args[0], .subset = expression.id}};
}

}  // namespace mitzi

using namespace mitzi;

template <class F>
struct run_static {
  constexpr run_static(F) {}
  inline static auto result = [] {
    F{}();
    return F{};
  }();
};

void foo(int arg) {
  recorder_stack::push();

  var<std::vector<int>> vec = {1, 2, 3};
  var index = arg;
  var v = 1;
  var ptr = &vec.ref()[index.ref()];
  var pcout = &std::cout;

  if_ (v.ref() > index.ref()) scope_ {
    vec.mut().push_back(v.ref());
  } else_ { scope_ 
    (*pcout.ref()) << *ptr.ref();
  } end_

  // uncommenting this will fail the borrow checker
  // (*pcout.ref()) << *ptr.ref();

  return;

  static constexpr auto ir = recorder_stack::get_ir();
  recorder_stack::pop();

  static_assert(nll_borrow_checker{}.validate(ir), "borrow checker failed");

  decltype(run_static([] {
    // uncommenting this will print the IR
    // mitzi::print(ir);
  }))::result;
}

int main() {
	return 0;
}

