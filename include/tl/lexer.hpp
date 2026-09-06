#pragma once

#include <cctype>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <vector>

namespace tl {

// The surface language is index notation, close to what a paper writes:
//
//   C[m,n] = sum(k, A[m,k] * B[k,n])
//
// Index notation is the right level to compile from because the loop nest is
// implicit in the indices rather than committed to by the author. A[m,k] used
// with B[k,n] says k is contracted and m,n are free -- which is exactly the
// information a scheduler needs, and exactly the information a hand-written
// triple loop has already thrown away.

enum class Tok {
  kIdent, kNumber, kSum,
  kLBracket, kRBracket, kLParen, kRParen,
  kComma, kEquals, kStar, kPlus, kMinus,
  kEnd,
};

struct Token {
  Tok kind = Tok::kEnd;
  std::string text;
  std::size_t pos = 0;
};

class LexError : public std::runtime_error {
 public:
  LexError(const std::string& msg, std::size_t pos)
      : std::runtime_error(msg + " at offset " + std::to_string(pos)), pos_(pos) {}
  std::size_t pos() const noexcept { return pos_; }
 private:
  std::size_t pos_;
};

inline std::vector<Token> lex(const std::string& src) {
  std::vector<Token> out;
  std::size_t i = 0;
  const std::size_t n = src.size();

  while (i < n) {
    const char c = src[i];
    if (std::isspace(static_cast<unsigned char>(c))) { ++i; continue; }

    const std::size_t start = i;
    if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
      while (i < n && (std::isalnum(static_cast<unsigned char>(src[i])) || src[i] == '_')) { ++i; }
      std::string word = src.substr(start, i - start);
      // `sum` is the only reserved word. Keeping the keyword set this small
      // means an identifier can be almost anything, which matters because
      // tensor and index names come from the user's own notation.
      out.push_back({word == "sum" ? Tok::kSum : Tok::kIdent, std::move(word), start});
      continue;
    }
    if (std::isdigit(static_cast<unsigned char>(c))) {
      while (i < n && (std::isdigit(static_cast<unsigned char>(src[i])) || src[i] == '.')) { ++i; }
      out.push_back({Tok::kNumber, src.substr(start, i - start), start});
      continue;
    }

    ++i;
    switch (c) {
      case '[': out.push_back({Tok::kLBracket, "[", start}); break;
      case ']': out.push_back({Tok::kRBracket, "]", start}); break;
      case '(': out.push_back({Tok::kLParen,   "(", start}); break;
      case ')': out.push_back({Tok::kRParen,   ")", start}); break;
      case ',': out.push_back({Tok::kComma,    ",", start}); break;
      case '=': out.push_back({Tok::kEquals,   "=", start}); break;
      case '*': out.push_back({Tok::kStar,     "*", start}); break;
      case '+': out.push_back({Tok::kPlus,     "+", start}); break;
      case '-': out.push_back({Tok::kMinus,    "-", start}); break;
      default:
        throw LexError(std::string("unexpected character '") + c + "'", start);
    }
  }
  out.push_back({Tok::kEnd, "", n});
  return out;
}

}  // namespace tl
