# Actual 3122F0 solver hull barycentric projection

The complete 242-site Primary body has SHA-256
`2f73873ee31d27fdd0bc1d8665c8e618c43930173e5cd6b90ee62382138228a7`.
The private typed kernel computes the original Gram terms and determinant.
For a nondegenerate triangle it uses the authored x87 numerator and
publication order. The alternate path evaluates the actual longest-edge
comparisons, including both BC output schedules and the zero route.
All six original epilogues restore the live saved frame.

GCC and Clang sanitizers each match 64 unchanged original calls, all
242 sites and all six epilogues, with 2,272 complete pre-I/O packets,
1,616 reads and 656 stores. The fixtures cover four free x87 slots,
different FP modes, paired packed NaNs, a determinant/output alias
and a saved-BP alias. Both compilers reject three deliberate defects
in the determinant, normal weight output and degenerate output.
See `build/test-results/native-solver-hull-barycentric-frame-20261110/{clang,gcc}-r2.json`.

The independent current retained-source comparison passes the same
64 complete cases and 2,272 packets per compiler, plus 324 stopped
provider or observer prefixes spanning all six epilogues. It keeps the
authored retained FST and FSTP publication ordering separate from the
staged raw physical protocol. See
`build/test-results/native-solver-hull-barycentric-source-20261110/{clang,gcc}-current-r2.json`.
All generated scratch trees were removed. The private comparison does
not establish fresh generator transport, physical fault atomicity,
another image, a production default or complete `313BB0` closure.
