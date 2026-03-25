#pragma once

#include <vector>
#include "../ir.h"

namespace mitzi {

class lexical_borrow_checker {
  struct borrow_state {
    int ref_count = 0;
    int mut_count = 0;
  };

  std::vector<std::vector<borrow_state>> scope_stack;
  std::vector<bool> is_pointer;
  std::vector<bool> is_var;
  std::vector<int> pointer_depths;

  constexpr bool impl(std::span<ir::instruction> records) { 
    scope_stack.emplace_back();
    for (int i = 0; i < records.size(); i++) {
      auto record = records[i];
      if (auto cf = record.get_if<ir::control_flow>()) {
        if (*cf == ir::control_flow::start) {
          auto last = scope_stack.back();
          scope_stack.emplace_back(std::move(last));
        } else if (*cf == ir::control_flow::end) {
          scope_stack.pop_back();
        }
      } else if (auto exp = record.get_if<ir::exp>()) {
        if (is_pointer[exp->type]) {
          is_pointer[exp->id] = true;
        }
        if (exp->fn_name.view() == "@constructor" && is_pointer[exp->id]) {
          pointer_depths[exp->id] = scope_stack.size();
        } else if ((exp->fn_name.view() == ".ref" ||
                    exp->fn_name.view() == ".mut") &&
                   exp->arg_count == 1 && is_pointer[exp->args[0]]) {
          pointer_depths[exp->id] = pointer_depths[exp->args[0]];
        } 
        else if (exp->fn_name.view() == ".operator=" &&
                   pointer_depths[exp->args[0]]) {
          auto depthDst = pointer_depths[exp->args[0]];
          auto depthSrc = pointer_depths[exp->args[1]];
          if (depthDst < depthSrc) {
            return false;
          }
        }

        if (exp->fn_name.view() == "@constructor") {
          is_var[exp->id] = true;
        }
        if (exp->arg_count == 1 && is_var[exp->args[0]]) {
          auto id = exp->args[0];
          auto& scope = scope_stack.back();
          if (scope.size() <= id) {
            scope.resize(id + 1);
          }
          auto& state = scope[id];
          if (exp->fn_name.view() == ".mut") {
            if (state.mut_count || state.ref_count) {
              return false;
            }
            state.mut_count++;
          } else if (exp->fn_name.view() == ".ref") {
            if (state.mut_count) {
              return false;
            }
            state.ref_count++;
          }
        }

      }
    }
    return true;
  }

public:

  constexpr bool validate(auto fn_info) { 
    auto records = fn_info.get_instructions();
    auto ids = fn_info.get_ids();

    is_pointer.resize(ids.size());
    is_var.resize(ids.size());
    pointer_depths.resize(ids.size());

    for_each_type_indexed_unwrap(
      [&](auto type, auto idx) {
        if constexpr (type_wrapper_c<decltype(type.get())>) {
          using T = decltype(type.get().get());
          is_pointer[idx.get()] = std::is_pointer_v<T>;
        }
      },
    ids);

    return impl(records);
  }
};

}
