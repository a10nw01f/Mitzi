#pragma once

#include <algorithm>
#include <vector>
#include <optional>
#include <bitset>
#include "../ir.h"
#include "../utils.h"

namespace mitzi {

struct lifetime_constraint {
  int superset;
  int subset;
};


constexpr auto get_lifetime_constraints(auto, auto, const ir::exp& expression) {
  return std::vector<lifetime_constraint>{};
}

class nll_borrow_checker {
 public:
  constexpr bool validate(auto ir) {
    collect_lifetime_constraints(ir);
    constexpr auto instructions = ir.get_instructions();
    constexpr auto ids = ir.get_ids();
    preprocess_program(instructions, ids.size());
    int cursor = 0;
    int initial_block = add_block();
    int final_block = parse_scope(instructions, initial_block, cursor);
    auto liveness = liveness_analysis<ids.size()>(cfg, instructions);
    auto regions = infer_regions(instructions, liveness);
    return borrow_check(instructions, regions, ids.size());
  }

 private:

  struct basic_block {
    std::vector<int> instructions;
    std::vector<int> successors;
  };

  enum class access_type { ref, mut };

  struct loan {
    int region;
    int path;
    access_type access;
  };

  static constexpr bool is_access_invalid(access_type access1,
                                          access_type access2) {
    return (access1 == access_type::mut || access2 == access_type::mut);
  }

  constexpr auto collect_lifetime_constraints(auto ir) {
    static constexpr auto instructions = ir.get_instructions();
    static constexpr auto ids = ir.get_ids();
    using ids_tuple = typename decltype(ir.get_ids())::template rename<std::tuple>;
    constexpr auto exp_id_to_type = []() {
      std::array<int, ids.size()> map = {};
      for (int i = 0; i < instructions.size(); i++) {
        if (auto exp = std::get_if<ir::exp>(&instructions[i])) {
          map[exp->id] = exp->type;
        }
      }
      return map;
    }();
    mitzi::for_each_index(
        [&](auto idx) {
          static constexpr auto inst = instructions[idx.get()];
          static constexpr auto exp = std::get_if<ir::exp>(&inst);
          if constexpr (exp) {
            expand_indices(
                [&](auto... is) {
                  using TL = type_list<
                      decltype(std::tuple_element_t<exp->type, ids_tuple>{}
                                   .get()
                                   .get()),
                      decltype(std::tuple_element_t<
                                   exp_id_to_type[exp->args[is.get()]],
                                   ids_tuple>{}
                                   .get()
                                   .get())...>;
                  for (auto constraint : get_lifetime_constraints(
                           value_wrapper<exp->fn_name>{}, TL{}, *exp)) {
                    this->constraints.emplace_back(constraint);
                  }
                },
                value_wrapper<exp->arg_count>{});
          }
        },
        value_wrapper<instructions.size()>{});
  }

  constexpr void preprocess_program(std::span<const ir::instruction> instructions,
                          int max_id) {
    is_used.resize(max_id);
    for (int i = 0; i < instructions.size(); i++) {
      auto& inst = instructions[i];
      auto exp = std::get_if<ir::exp>(&inst);
      if (!exp) {
        continue;
      }

      for (int j = 0; j < exp->arg_count; j++) {
        is_used[exp->args[j]] = true;
      }
    }
  }

  constexpr bool is_end_statement(const ir::instruction& instr) const {
    if (auto exp = std::get_if<ir::exp>(&instr)) {
      return exp->fn_name.view() == "@constructor" || !is_used[exp->id];
    }
    return true;
  }

  constexpr int parse_if_branch(std::span<const ir::instruction> instructions,
                      int parent_block_index,
                      int& cursor) {
    for (; cursor < instructions.size(); cursor++) {
      if (auto end = std::get_if<ir::control_flow>(&instructions[cursor]);
          end && *end == ir::control_flow::start) {
        break;
      }
      if (std::get_if<ir::exp>(&instructions[cursor])) {
        cfg[parent_block_index].instructions.emplace_back(cursor);
      }
    }

    int merged = add_block();
    int taken = add_block();
    int end_taken = parse_scope(instructions, taken, cursor);
    connect(parent_block_index, taken);
    connect(end_taken, merged);
    while (true) {
      auto cf = std::get_if<ir::control_flow>(&instructions[cursor]);
      if (!cf || (*cf != ir::control_flow::else_if)) {
        break;
      }
      cursor++;
      int skipped = add_block();
      int end_skipped = parse_scope(instructions, skipped, cursor);
      connect(parent_block_index, skipped);
      connect(end_skipped, merged);
      parent_block_index = skipped;
    }

    if (auto cf = std::get_if<ir::control_flow>(&instructions[cursor]);
        cf && *cf == ir::control_flow::_else) {
      cursor++;
      int skipped = add_block();
      int end_skipped = parse_scope(instructions, skipped, cursor);
      connect(parent_block_index, skipped);
      connect(end_skipped, merged);
    }

    return merged;
  }

  constexpr void parse_statement(std::span<const ir::instruction> instructions,
                       int parent_block_index,
                       int& cursor) {
    auto& block = cfg[parent_block_index];
    for (; cursor < instructions.size(); cursor++) {
      if (std::get_if<ir::exp>(&instructions[cursor])) {
        block.instructions.emplace_back(cursor);
      }
      if (is_end_statement(instructions[cursor])) {
        break;
      }
    }
  }

  constexpr int parse_for_loop(std::span<const ir::instruction> instructions,
                     int parent_block_index,
                     int& cursor) {
    int header = add_block();
    int body = add_block();
    int end_loop = add_block();
    int merged = add_block();

    connect(parent_block_index, header);
    connect(parent_block_index, merged);
    connect(header, body);
    connect(header, merged);
    connect(end_loop, header);

    parse_statement(instructions, parent_block_index, cursor);
    parse_statement(instructions, header, cursor);
    parse_statement(instructions, end_loop, cursor);
    int body_end = parse_scope(instructions, body, cursor);
    connect(body_end, end_loop);

    return merged;
  }

  constexpr int parse_scope(std::span<const ir::instruction> instructions,
                  int parent_block_index,
                  int& cursor) {
    auto current_block_index = parent_block_index;
    for (; cursor < instructions.size();) {
      auto& inst = instructions[cursor];
      if (const auto* cf = std::get_if<ir::control_flow>(&inst)) {
        if (*cf == ir::control_flow::_if) {
          cursor++;
          current_block_index =
              parse_if_branch(instructions, current_block_index, cursor);
        } else if (*cf == ir::control_flow::end) {
          cursor++;
          return current_block_index;
        } else if (*cf == ir::control_flow::_for) {
          cursor++;
          current_block_index =
              parse_for_loop(instructions, current_block_index, cursor);
        } else {
          cursor++;
        }
      } else if (const ir::exp* exp = std::get_if<ir::exp>(&inst)) {
        cfg[current_block_index].instructions.emplace_back(cursor);
        cursor++;
      } else {
        cursor++;
      }
    }
    return current_block_index;
  }

  template <auto N>
  using var_set = std::bitset<N>;

  template <auto N>
  constexpr std::vector<var_set<N>> liveness_analysis(
      const std::vector<basic_block>& graph,
      std::span<const mitzi::ir::instruction> all_instrs) {
    auto num_blocks = graph.size();

    std::vector<var_set<N>> block_gen(num_blocks, var_set<N>());
    std::vector<var_set<N>> block_kill(num_blocks, var_set<N>());

    for (int i = 0; i < num_blocks; ++i) {
      for (int instr_idx : graph[i].instructions) {
        const auto& instr = *std::get_if<mitzi::ir::exp>(&all_instrs[instr_idx]);

        for (int src : instr.get_args()) {
          if (!block_kill[i][src]) {
            block_gen[i][src] = true;
          }
        }

        block_kill[i][instr.id] = true;
      }
    }

    std::vector<var_set<N>> live_in(num_blocks, var_set<N>());
    std::vector<var_set<N>> live_out(num_blocks, var_set<N>());
    bool changed = true;

    while (changed) {
      changed = false;
      for (int i = int(num_blocks) - 1; i >= 0; --i) {
        var_set<N> new_live_out = {};
        for (int succ : graph[i].successors) {
          for (int v = 0; v < N; ++v) {
            if (live_in[succ][v])
              new_live_out[v] = true;
          }
        }
        live_out[i] = new_live_out;

        var_set<N> new_live_in = block_gen[i];
        for (int v = 0; v < N; ++v) {
          if (live_out[i][v] && !block_kill[i][v]) {
            new_live_in[v] = true;
          }
        }

        if (new_live_in != live_in[i]) {
          live_in[i] = new_live_in;
          changed = true;
        }
      }
    }

    std::vector<var_set<N>> result;
    result.resize(all_instrs.size());

    for (int i = 0; i < num_blocks; ++i) {
      const auto& block = graph[i];

      auto current_live = live_out[i];

      for (int j = static_cast<int>(block.instructions.size()) - 1; j >= 0;
           --j) {
        result[block.instructions[j]] = current_live;

        const auto& instr =
            *std::get_if<mitzi::ir::exp>(&all_instrs[block.instructions[j]]);
        current_live[instr.id] = false;
        for (int src : instr.get_args()) {
          current_live[src] = true;
        }
      }
    }

    return result;
  }

  template<auto N>
  constexpr std::vector<var_set<N>> infer_regions(
      std::span<const ir::instruction> instructions,
      const std::vector<var_set<N>>& liveness) {
    auto result = liveness;
    bool changed = true;
    while (changed) {
      changed = false;
      for (auto& currLive : result) {
        for (auto constraint : this->constraints) {
          if (currLive[constraint.subset] && !currLive[constraint.superset]) {
            currLive[constraint.superset] = true;
            changed = true;
          }
        }
      }
    }

    return result;
  }

  template <auto N>
  constexpr bool borrow_check(std::span<const ir::instruction> instructions,
                              const std::vector<var_set<N>>& regions,
                              int max_id) {
    std::vector<loan> loans;
    std::vector<bool> is_var(max_id);
    for (int i = 0; i < instructions.size(); i++) {
      if (auto exp = std::get_if<mitzi::ir::exp>(&instructions[i])) {
        if (exp->fn_name.view() == "@constructor") {
          is_var[exp->id] = true;
          continue;
        }

        std::optional<access_type> access;
        int path = -1;
        if (exp->fn_name.view() == ".ref" && is_var[exp->args[0]]) {
          access = access_type::ref;
        } else if (exp->fn_name.view() == ".mut" && is_var[exp->args[0]]) {
          access = access_type::mut;
        }

        if (!access.has_value()) {
          continue;
        }

        auto new_loan =
            loan{.region = exp->id, .path = exp->args[0], .access = *access};

        for (auto& other : loans) {
          if (regions[i][other.region] && other.path == new_loan.path &&
              is_access_invalid(other.access, new_loan.access)) {
            return false;
          }
        }
        loans.emplace_back(new_loan);
      }
    }

    return true;
  }

  constexpr int add_block() {
    int index = cfg.size();
    cfg.emplace_back();
    return index;
  }

  constexpr void connect(int parent, int successor) {
    cfg[parent].successors.emplace_back(successor);
  }

  std::vector<basic_block> cfg;
  struct region_info {
    int def_block = -1;
    int last_use_key = -1;
    std::vector<bool> live_blocks = {};
  };
  struct active_borrows {
    std::vector<int> shared;
    std::vector<int> mut;
  };
  std::vector<bool> is_used;
  std::vector<lifetime_constraint> constraints;
  std::vector<region_info> regions;
};

}  // namespace mitzi