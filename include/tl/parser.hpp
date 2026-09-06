#pragma once

#include <set>
#include <stdexcept>
#include <string>
#include <vector>

#include "tl/ast.hpp"
#include "tl/lexer.hpp"

namespace tl {

class ParseError : public std::runtime_error {
 public:
  ParseError(const std::string& msg, std::size_t pos)
      : std::runtime_error(msg + " at offset " + std::to_string(pos)) {}
};

// Recursive descent, precedence climbing for the two levels that exist:
// multiplication binds tighter than addition. Hand-written rather than
// generated because the grammar is small enough that a parser generator would
// cost more in build complexity than it saves, and because error messages from
// hand-written parsers are the ones people can actually act on.
class Parser {
 public:
  explicit Parser(const std::string& src) : toks_(lex(src)) {}

  Statement parse_statement() {
    Statement st;
    st.output = expect_ident("expected output tensor name");
    st.output_indices = parse_index_list();
    expect(Tok::kEquals, "expected '=' after output indices");
    st.rhs = parse_add();
    expect(Tok::kEnd, "trailing tokens after expression");
    validate(st);
    return st;
  }

 private:
  std::vector<Token> toks_;
  std::size_t i_ = 0;

  const Token& peek() const { return toks_[i_]; }
  const Token& advance() { return toks_[i_++]; }
  bool match(Tok k) { if (peek().kind == k) { ++i_; return true; } return false; }

  void expect(Tok k, const char* msg) {
    if (peek().kind != k) { throw ParseError(msg, peek().pos); }
    ++i_;
  }
  std::string expect_ident(const char* msg) {
    if (peek().kind != Tok::kIdent) { throw ParseError(msg, peek().pos); }
    return advance().text;
  }

  std::vector<std::string> parse_index_list() {
    expect(Tok::kLBracket, "expected '[' to open an index list");
    std::vector<std::string> idx;
    if (peek().kind != Tok::kRBracket) {
      do { idx.push_back(expect_ident("expected an index name")); } while (match(Tok::kComma));
    }
    expect(Tok::kRBracket, "expected ']' to close the index list");
    return idx;
  }

  ExprPtr parse_add() {
    ExprPtr lhs = parse_mul();
    while (peek().kind == Tok::kPlus || peek().kind == Tok::kMinus) {
      const BinOp op = advance().kind == Tok::kPlus ? BinOp::kAdd : BinOp::kSub;
      lhs = make_binary(op, std::move(lhs), parse_mul());
    }
    return lhs;
  }

  ExprPtr parse_mul() {
    ExprPtr lhs = parse_atom();
    while (peek().kind == Tok::kStar) {
      advance();
      lhs = make_binary(BinOp::kMul, std::move(lhs), parse_atom());
    }
    return lhs;
  }

  ExprPtr parse_atom() {
    if (match(Tok::kLParen)) {
      ExprPtr inner = parse_add();
      expect(Tok::kRParen, "expected ')'");
      return inner;
    }
    if (peek().kind == Tok::kSum) {
      const std::size_t pos = advance().pos;
      expect(Tok::kLParen, "expected '(' after sum");
      std::string index = expect_ident("expected the index to sum over");
      expect(Tok::kComma, "expected ',' after the sum index");
      ExprPtr body = parse_add();
      expect(Tok::kRParen, "expected ')' to close sum");
      if (index.empty()) { throw ParseError("empty sum index", pos); }
      return make_sum(std::move(index), std::move(body));
    }
    if (peek().kind == Tok::kNumber) {
      return make_number(std::stod(advance().text));
    }
    if (peek().kind == Tok::kIdent) {
      std::string name = advance().text;
      return make_access(std::move(name), parse_index_list());
    }
    throw ParseError("expected an expression", peek().pos);
  }

  static void collect_free(const Expr& e, std::set<std::string>& free) {
    switch (e.kind) {
      case ExprKind::kAccess:
        for (const auto& idx : e.indices) { free.insert(idx); }
        break;
      case ExprKind::kBinary:
        collect_free(*e.lhs, free); collect_free(*e.rhs, free);
        break;
      case ExprKind::kSum: {
        std::set<std::string> inner;
        collect_free(*e.body, inner);
        // A summed index is bound by the sum and is not free outside it.
        inner.erase(e.reduce_index);
        free.insert(inner.begin(), inner.end());
        break;
      }
      case ExprKind::kNumber: break;
    }
  }

  // Every index on the left must be free on the right. Catching this here is
  // the difference between a clear message and a lowering pass that emits a
  // loop over an index nothing indexes.
  static void validate(const Statement& st) {
    std::set<std::string> free;
    collect_free(*st.rhs, free);
    for (const auto& idx : st.output_indices) {
      if (free.find(idx) == free.end()) {
        throw ParseError("output index '" + idx + "' does not appear free on the right-hand side", 0);
      }
    }
  }
};

inline Statement parse(const std::string& src) { return Parser(src).parse_statement(); }

}  // namespace tl
