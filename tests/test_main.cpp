#include <cassert>
#include <cstdio>
#include <string>

#include "tl/parser.hpp"

namespace {

// Parse, print, parse again. If the two printings differ, the parser and the
// printer disagree about the tree, and one of them is wrong. This catches
// precedence and associativity bugs that a "does it parse" test never will.
void expect_round_trip(const std::string& src, const std::string& expected) {
  const tl::Statement first = tl::parse(src);
  const std::string once = tl::to_string(first);
  assert(once == expected);

  const tl::Statement second = tl::parse(once);
  assert(tl::to_string(second) == once);
}

void expect_parse_error(const std::string& src) {
  bool threw = false;
  try {
    tl::parse(src);
  } catch (const std::runtime_error&) {
    threw = true;
  }
  assert(threw);
}

void test_matmul() {
  expect_round_trip("C[m,n] = sum(k, A[m,k] * B[k,n])",
                    "C[m,n] = sum(k, (A[m,k] * B[k,n]))");
}

void test_elementwise() {
  expect_round_trip("D[i] = A[i] + B[i] * C[i]",
                    "D[i] = (A[i] + (B[i] * C[i]))");
}

// Multiplication must bind tighter than addition. Printed parens make the
// association visible rather than assumed.
void test_precedence() {
  expect_round_trip("D[i] = A[i] * B[i] + C[i]",
                    "D[i] = ((A[i] * B[i]) + C[i])");
  expect_round_trip("D[i] = A[i] * (B[i] + C[i])",
                    "D[i] = (A[i] * (B[i] + C[i]))");
}

void test_nested_reduction() {
  expect_round_trip("E[m] = sum(k, sum(j, A[m,k] * B[k,j]))",
                    "E[m] = sum(k, sum(j, (A[m,k] * B[k,j])))");
}

void test_scalar_literal() {
  expect_round_trip("B[i] = A[i] * 2", "B[i] = (A[i] * 2)");
}

// A summed index is bound by its sum. Using it on the left is a real error and
// should be caught at parse time, not when a lowering pass tries to build a
// loop over an index that no longer exists.
void test_summed_index_is_not_free() {
  expect_parse_error("C[m,k] = sum(k, A[m,k] * B[k,n])");
}

void test_unknown_output_index_rejected() {
  expect_parse_error("C[m,z] = sum(k, A[m,k] * B[k,n])");
}

void test_syntax_errors() {
  expect_parse_error("C[m,n] = ");
  expect_parse_error("C[m,n] sum(k, A[m,k])");
  expect_parse_error("C[m,n] = sum(k A[m,k])");
  expect_parse_error("C[m,n] = A[m,k");
  expect_parse_error("C[m,n] = A[m,k] * B[k,n] $");
  expect_parse_error("C[m,n] = sum(, A[m,k])");
}

// Zero-index tensors are legal: a scalar output from a full contraction.
void test_scalar_output() {
  expect_round_trip("s[] = sum(i, A[i] * B[i])", "s[] = sum(i, (A[i] * B[i]))");
}

}  // namespace

int main() {
  test_matmul();
  test_elementwise();
  test_precedence();
  test_nested_reduction();
  test_scalar_literal();
  test_summed_index_is_not_free();
  test_unknown_output_index_rejected();
  test_syntax_errors();
  test_scalar_output();
  std::printf("all tests passed\n");
  return 0;
}
