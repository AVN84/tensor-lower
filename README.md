# tensor-lower

A small compiler that takes tensor expressions in index notation and lowers
them to the quantized kernels in
[quantized-inference-kernels](https://github.com/AVN84/quantized-inference-kernels).

```
C[m,n] = sum(k, A[m,k] * B[k,n])
```

## Why index notation

A hand-written triple loop has already committed to a loop order, a tiling, and
a memory layout — and it has thrown away the information needed to choose them
differently. Index notation keeps that information: `A[m,k]` used against
`B[k,n]` says `k` is contracted while `m` and `n` are free. That is precisely
what a scheduler needs and precisely what the loop nest destroys.

The bet is that going from this notation down to the NEON and Metal kernels next
door should be mechanical, and that the interesting decisions — tile sizes,
which axis to vectorize, when int4 is safe — belong to a pass rather than to
whoever typed the loops.

## Status

**Front end works. Nothing lowers yet.**

Done:
- Lexer, hand-written, one reserved word
- Recursive-descent parser with precedence climbing
- AST for accesses, binary ops, literals, and contractions
- Free-index analysis: an index bound by a `sum` is not free outside it
- Validation that every output index appears free on the right
- Round-trip tests — parse, print, parse again, compare

Not done:
- Lowering to loop nests
- Tiling and vectorization passes
- Code emission to NEON / Metal
- Shape inference and bounds

## Design notes

**The grammar is deliberately tiny.** Four expression kinds — access, binary op,
number, contraction. A GEMM, a convolution written as an index sum, and
elementwise fusion all fit. Adding cases before a lowering pass needs them is
how an IR ends up unschedulable.

**Hand-written parser, not generated.** The grammar is small enough that a
parser generator costs more build complexity than it saves, and hand-written
parsers give error messages people can act on.

**Round-trip tests over "does it parse".** Every test parses, prints, and
re-parses, then compares. A parser that cannot reproduce what it read has bugs
that a parse-succeeds test will never surface — precedence and associativity in
particular.

**Bound indices are checked at parse time.** `C[m,k] = sum(k, ...)` is rejected,
because `k` is bound by the sum and cannot also be an output index. Catching it
here is the difference between a clear message and a lowering pass emitting a
loop over an index that no longer exists.

## Build

```
make test       # parser + validation tests
make sanitize   # ASan + UBSan
```

C++20, no dependencies.

## Next

1. Lower a statement to an explicit loop nest over free and reduction indices
2. Shape inference from declared tensor dimensions
3. Tiling pass, with the tile sizes the int8 kernels already found useful (4x4)
4. Emit C++ that calls into `quantized-inference-kernels`
5. Metal emission after that
