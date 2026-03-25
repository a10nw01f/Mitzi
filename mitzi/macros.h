#pragma once

#include "exp.h"
#include "utils.h"

#define WRAP_BINARY_OP(OP)                                                    \
  auto operator OP(this mitzi::exp_c auto&& self, mitzi::exp_c auto&& other) { \
    return mitzi::exp(                                                         \
        mitzi::value_wrapper<mitzi::ir::func_name("operator" #OP)>{},          \
        [](auto&& a, auto&& b) -> decltype(auto) { return FWD(a) OP FWD(b); }, \
        self, other);                                                          \
  }

#define WRAP_BINARY_OP_EX(OP, EX)                                              \
  auto operator OP(this mitzi::exp_c auto&& self, mitzi::exp_c auto&& other) { \
    return mitzi::exp(                                                         \
        mitzi::value_wrapper<mitzi::ir::func_name("operator" #OP)>{},          \
        [](auto&& a, auto&& b) -> decltype(auto) { return EX; }, self, other); \
  }

#define WRAP_UNARY_OP(OP)                                            \
  auto operator OP(this mitzi::exp_c auto&& self) {                   \
    return mitzi::exp(                                                \
        mitzi::value_wrapper<mitzi::ir::func_name("operator" #OP)>{}, \
        [](auto&& a) -> decltype(auto) { return OP FWD(a); }, self);  \
  }

#define WRAP_CAST(OP)                                                 \
  operator OP(this mitzi::exp_c auto&& self) {                        \
    return mitzi::exp(                                                \
        mitzi::value_wrapper<mitzi::ir::func_name("operator" #OP)>{}, \
        [](auto&& a) -> decltype(auto) { return (OP)(FWD(a)); }, self);                   \
  }

#define WRAP_METHOD(METHOD)                                                  \
  auto METHOD(this mitzi::exp_c auto&& self, mitzi::exp_c auto&&... rest) {   \
    return mitzi::exp(                                                        \
        mitzi::value_wrapper<mitzi::ir::func_name("." #METHOD)>{},            \
        [](auto&& a, auto&&... args) -> decltype(auto) {                    \
          return FWD(a).METHOD(FWD(args)...);                               \
        }, \
        self, rest...);                                                       \
  }

#define WRAP_ASSIGNEMNT_OP() \
auto assignment_operator(this mitzi::exp_c auto&& self, mitzi::exp_c auto&& other) noexcept {\
          return mitzi::exp(                                                  \
              mitzi::value_wrapper<mitzi::ir::func_name(".operator=")>{},     \
              [](auto&& a, auto&& b) -> decltype(auto) {                      \
                return std::forward<decltype(a)>(a) =                         \
                            std::forward<decltype(b)>(b);                     \
              },                                                              \
              self, other);                                                   \
        }
