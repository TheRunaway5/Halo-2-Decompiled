# Actual 314A30 solver hull facet plane

The complete 66-site Primary body has SHA-256
`709b82327b9db90dcc38e86ec52e3f3c5122042bab49218b0108f3b2639cbd61`.
The private typed schedule constructs the triangle normal, performs the
real scalar reciprocal-square-root estimate and authored refinement,
publishes both unnormalized and normalized vectors, and rereads the
first vertex before publishing the plane distance. Both scratch reloads
and the first-vertex reread observe live aliases.

Both sanitizer compilers match 64 unchanged i386 cases, all 66
instructions, 1,472 full pre-I/O/estimate packets, 832 reads and
576 architectural stores. The three authored 16-byte stores per case
are compared as their declared ordered 8+8 provider events. Both
compilers reject reversed normal orientation, the wrong estimate
destination and omission of the first normal publication. This
ordinary-backed comparison does not qualify physical partial writes,
saved RF on alignment faults or unmasked FP. See
`build/test-results/native-solver-hull-facet-plane-frame-20261110/`.

The retained-source comparison uses the real generated estimate helper
and the runtime execution-input journal. Each compiler matches 64
cases, 1,472 complete packets, 400 stopped prefixes, eight real
capture/replay pairs, eight wrong-operand refusals and eight missing
estimate-owner refusals. The three compiled defects change the actual
estimate operand, its preserved upper lanes or the ordered NaN result.

The original GCC retained expression has a recorded counterexample:
the W lane at the plane-distance spill is `0xFFC00001` rather than
`0x7FC12345`. An unchanged i386 single-case witness verifies the
destination-first `MULPS XMM0,XMM1` at `314AEF` (bytes `0F59C1`)
and the resulting `0x7FC12345` lane at `314B0C`. The phased
reference used for both final source comparisons preserves operand
order at that exact site only. The unmodified Clang source also passes.
The original GCC failure remains separate; no canonical source is
changed. Receipts and the single-case witness are in
`build/test-results/native-solver-hull-facet-plane-source-20261110/`.
All owned trees are removed. The owner remains private and unbound.
