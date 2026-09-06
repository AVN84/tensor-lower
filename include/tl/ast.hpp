#pragma once

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace tl {

// Expression nodes. Deliberately small: an access into a tensor, a binary
// operation, a literal, and a contraction. Everything a GEMM, a convolution
// expressed as an index sum, or an elementwise fusion needs is expressible in
// those four, and adding cases before there is a lowering pass that needs them
// is how a compiler ends up with an IR nobody can schedule.

struct Expr;
using ExprPtr = std::unique_ptr<Expr>;

enum class ExprKind { kAccess, kBinary, kNumber, kSum };
enum class BinOp { kMul, kAdd, kSub };

struct Expr {
  ExprKind kind;

  // kAccess: A[m,k]
  std::string tensor;
  std::vector<std::string> indices;

  // kBinary
  BinOp op = BinOp::kMul;
  ExprPtr lhs, rhs;

  // kNumber
  double value = 0.0;

  // kSum: the index being contracted, and the body summed over it.
  std::string reduce_index;
  ExprPtr body;

  explicit Expr(ExprKind k) : kind(k) {}
};

inline ExprPtr make_access(std::string tensor, std::vector<std::string> idx) {
  auto e = std::make_unique<Expr>(ExprKind::kAccess);
  e->tensor = std::move(tensor);
  e->indices = std::move(idx);
  return e;
}

inline ExprPtr make_binary(BinOp op, ExprPtr l, ExprPtr r) {
  auto e = std::make_unique<Expr>(ExprKind::kBinary);
  e->op = op;
  e->lhs = std::move(l);
  e->rhs = std::move(r);
  return e;
}

inline ExprPtr make_number(double v) {
  auto e = std::make_unique<Expr>(ExprKind::kNumber);
  e->value = v;
  return e;
}

inline ExprPtr make_sum(std::string index, ExprPtr body) {
  auto e = std::make_unique<Expr>(ExprKind::kSum);
  e->reduce_index = std::move(index);
  e->body = std::move(body);
  return e;
}

// A whole statement: one output tensor with its free indices, and the
// expression producing it.
struct Statement {
  std::string output;
  std::vector<std::string> output_indices;
  ExprPtr rhs;
};

// Round-trippable text form, used by the tests. A parser that cannot print
// back what it read is a parser whose bugs are invisible.
inline std::string to_string(const Expr& e) {
  switch (e.kind) {
    case ExprKind::kNumber: {
      std::string s = std::to_string(e.value);
      // Trim trailing zeros so 2.0 prints as "2" rather than "2.000000".
      while (s.size() > 1 && s.back() == '0') { s.pop_back(); }
      if (!s.empty() && s.back() == '.') { s.pop_back(); }
      return s;
    }
    case ExprKind::kAccess: {
      std::string s = e.tensor + "[";
      for (std::size_t i = 0; i < e.indices.size(); ++i) {
        if (i) { s += ","; }
        s += e.indices[i];
      }
      return s + "]";
    }
    case ExprKind::kBinary: {
      const char* op = e.op == BinOp::kMul ? " * " : (e.op == BinOp::kAdd ? " + " : " - ");
      return "(" + to_string(*e.lhs) + op + to_string(*e.rhs) + ")";
    }
    case ExprKind::kSum:
      return "sum(" + e.reduce_index + ", " + to_string(*e.body) + ")";
  }
  return "?";
}

inline std::string to_string(const Statement& s) {
  std::string out = s.output + "[";
  for (std::size_t i = 0; i < s.output_indices.size(); ++i) {
    if (i) { out += ","; }
    out += s.output_indices[i];
  }
  return out + "] = " + to_string(*s.rhs);
}

}  // namespace tl
