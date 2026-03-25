#pragma once

#include <cassert>
#include <cstddef>
#include <memory>
#include <span>
#include <vector>
#include "ir.h"

namespace ast {
struct node {
  virtual ~node() = default;
};

struct statement : node {
  std::vector<int> expressions;
};

struct scope : node {
  std::vector<std::unique_ptr<node>> nodes;
};

struct branch : node {
  scope taken;
  scope skipped;
};

struct loop : node {
  scope header;
  scope body;
};

namespace detail {

inline const mitzi::ir::exp* as_exp(const mitzi::ir::instruction& inst) {
  return inst.get_if<mitzi::ir::exp>();
}

inline const mitzi::ir::control_flow* as_cf(const mitzi::ir::instruction& inst) {
  return inst.get_if<mitzi::ir::control_flow>();
}

inline void append_exp_statement(scope& s, const mitzi::ir::exp& e) {
  auto st = std::make_unique<statement>();
  st->expressions.push_back(e.id);
  s.nodes.push_back(std::move(st));
}

void parse_scope_nodes(scope& s, std::span<const mitzi::ir::instruction> ir,
                       std::size_t& i, bool stop_at_scope_end);

inline std::unique_ptr<branch> parse_else_if_chain(
    std::span<const mitzi::ir::instruction> ir, std::size_t& i) {
  assert(i < ir.size());
  assert(as_cf(ir[i]) &&
         *as_cf(ir[i]) == mitzi::ir::control_flow::else_if);
  ++i;
  auto br = std::make_unique<branch>();
  while (i < ir.size() && as_exp(ir[i])) {
    append_exp_statement(br->taken, *as_exp(ir[i]));
    ++i;
  }
  assert(i < ir.size() && as_cf(ir[i]) &&
         *as_cf(ir[i]) == mitzi::ir::control_flow::start);
  ++i;
  auto body = std::make_unique<scope>();
  parse_scope_nodes(*body, ir, i, true);
  assert(i < ir.size() && as_cf(ir[i]) &&
         *as_cf(ir[i]) == mitzi::ir::control_flow::end);
  ++i;
  br->taken.nodes.push_back(std::move(body));
  if (i < ir.size() && as_cf(ir[i]) &&
      *as_cf(ir[i]) == mitzi::ir::control_flow::else_if) {
    br->skipped.nodes.push_back(
        std::unique_ptr<node>(parse_else_if_chain(ir, i)));
  }
  return br;
}

inline std::unique_ptr<branch> parse_if_statement(
    std::span<const mitzi::ir::instruction> ir, std::size_t& i) {
  assert(i < ir.size());
  assert(as_cf(ir[i]) && *as_cf(ir[i]) == mitzi::ir::control_flow::_if);
  ++i;
  auto br = std::make_unique<branch>();
  while (i < ir.size() && as_exp(ir[i])) {
    append_exp_statement(br->taken, *as_exp(ir[i]));
    ++i;
  }
  assert(i < ir.size() && as_cf(ir[i]) &&
         *as_cf(ir[i]) == mitzi::ir::control_flow::start);
  ++i;
  auto body = std::make_unique<scope>();
  parse_scope_nodes(*body, ir, i, true);
  assert(i < ir.size() && as_cf(ir[i]) &&
         *as_cf(ir[i]) == mitzi::ir::control_flow::end);
  ++i;
  br->taken.nodes.push_back(std::move(body));
  if (i < ir.size() && as_cf(ir[i]) &&
      *as_cf(ir[i]) == mitzi::ir::control_flow::else_if) {
    br->skipped.nodes.push_back(
        std::unique_ptr<node>(parse_else_if_chain(ir, i)));
  }
  return br;
}

inline std::unique_ptr<loop> parse_for_loop(
    std::span<const mitzi::ir::instruction> ir, std::size_t& i) {
  assert(i < ir.size());
  assert(as_cf(ir[i]) && *as_cf(ir[i]) == mitzi::ir::control_flow::_for);
  ++i;
  auto lp = std::make_unique<loop>();
  while (i < ir.size() && as_exp(ir[i])) {
    append_exp_statement(lp->header, *as_exp(ir[i]));
    ++i;
  }
  assert(i < ir.size() && as_cf(ir[i]) &&
         *as_cf(ir[i]) == mitzi::ir::control_flow::start);
  ++i;
  parse_scope_nodes(lp->body, ir, i, true);
  assert(i < ir.size() && as_cf(ir[i]) &&
         *as_cf(ir[i]) == mitzi::ir::control_flow::end);
  ++i;
  return lp;
}

inline void parse_scope_nodes(scope& s,
                              std::span<const mitzi::ir::instruction> ir,
                              std::size_t& i, bool stop_at_scope_end) {
  while (i < ir.size()) {
    const auto& inst = ir[i];
    if (stop_at_scope_end && as_cf(inst) &&
        *as_cf(inst) == mitzi::ir::control_flow::end) {
      return;
    }
    if (inst.get_if<mitzi::ir::start>()) {
      ++i;
      continue;
    }
    if (auto* e = as_exp(inst)) {
      append_exp_statement(s, *e);
      ++i;
      continue;
    }
    if (auto* cf = as_cf(inst)) {
      switch (*cf) {
        case mitzi::ir::control_flow::start: {
          ++i;
          auto inner = std::make_unique<scope>();
          parse_scope_nodes(*inner, ir, i, true);
          assert(i < ir.size() && as_cf(ir[i]) &&
                 *as_cf(ir[i]) == mitzi::ir::control_flow::end);
          ++i;
          s.nodes.push_back(std::move(inner));
          break;
        }
        case mitzi::ir::control_flow::_if:
          s.nodes.push_back(
              std::unique_ptr<node>(parse_if_statement(ir, i)));
          break;
        case mitzi::ir::control_flow::_for:
          s.nodes.push_back(std::unique_ptr<node>(parse_for_loop(ir, i)));
          break;
        case mitzi::ir::control_flow::_return:
          ++i;
          break;
        case mitzi::ir::control_flow::end:
          assert(!stop_at_scope_end && "unbalanced control_flow::end");
          ++i;
          break;
        case mitzi::ir::control_flow::else_if:
          assert(false && "else_if without preceding if");
          return;
      }
      continue;
    }
    ++i;
  }
}

}  // namespace detail

inline scope create_ast(std::span<const mitzi::ir::instruction> ir) {
  scope root;
  std::size_t i = 0;
  if (!ir.empty() && ir[0].get_if<mitzi::ir::start>()) {
    ++i;
  }
  detail::parse_scope_nodes(root, ir, i, false);
  assert(i == ir.size());
  return root;
}

}  // namespace ast
