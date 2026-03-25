#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <span>
#include "../ir.h"
#include "../utils.h"
#include "../reflect.h"

namespace mitzi {
inline void print(std::span<const mitzi::ir::instruction> commands,
           const std::vector<std::string>& type_names) {
  int depth = 0;
  auto println = [&depth](auto&&... vs) {
    for (int i = 0; i < depth; i++) {
      std::cout << '\t';
    }
    (std::cout << ... << vs) << std::endl;
  };
  for (int i = 1; i < commands.size(); i++) {
    auto& cmd = commands[i];
    if (auto* cf = cmd.get_if<mitzi::ir::control_flow>()) {
      switch (*cf) {
        case mitzi::ir::control_flow::start:
          println("{");
          depth++;
          break;
        case mitzi::ir::control_flow::end:
          depth--;
          println("}");
          break;
        case mitzi::ir::control_flow::_if:
          println("if");
          break;
        case mitzi::ir::control_flow::else_if:
          println("else if");
          break;
        case mitzi::ir::control_flow::_for:
          println("for");
          break;
        case mitzi::ir::control_flow::_return:
          println("return");
          break;
        default:
          break;
      }
    }
    if (auto* exp = cmd.get_if<mitzi::ir::exp>()) {
      std::string fncall = "";
      int startArg = 0;
      if (exp->fn_name.c_str()[0] == '.') {
        fncall += "_" + std::to_string(exp->args[0]);
        startArg = 1;
      }
      fncall += exp->fn_name;
      fncall += '(';
      for (int i = startArg; i < exp->arg_count; i++) {
        if (i != startArg) {
          fncall += ", ";
        }

        fncall += "_" + std::to_string(exp->args[i]);
      }
      fncall += ')';

      if (exp->fn_name == std::string_view("@constructor")) {
        println("var _", std::to_string(exp->id), " : ", type_names[exp->type],
                " = ", fncall);
      } else {
        println("_", std::to_string(exp->id), " : ", type_names[exp->type],
                " = ", fncall);
      }
    }
  }
}

inline void print(auto fn_info) {
  auto ids = fn_info.get_ids();
  auto size = ids.size();
  std::vector<std::string> names(size);
  for_each_type_indexed_unwrap(
      [&](auto type, auto idx) {
        if constexpr (mitzi::type_wrapper_c<decltype(type.get())>) {
          using T = decltype(type.get().get());
          names[idx.get()] = mitzi::get_type_name<T>();
        }
      },
      ids);
  print(fn_info.get_instructions(), names);
}
}
