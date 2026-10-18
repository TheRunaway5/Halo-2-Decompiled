# Bank b11 goldens provenance

Captured 2026-08-24 from real MCPX APU GP silicon (xbtest oracle, one fresh
boot per case). First bank of gap-analysis round 2
(dsp-corpus-gap-analysis.md): the bit-field datapath with REAL widths (every
earlier extract/insert case mis-packed the width into control bits 11:8 and
ran width=0), RM truncation-rounding coverage (one case existed before), SM x
{adc, addl, unsigned multiplies, scaled rounding clamp}, reserved scaling mode
3, immediate-ALU B destinations, the DIV negative-divisor matrix, NORM/NORMF
path completion, AGU reserved-M / modulo-abuse / bit-reverse probes, SSL and
SP bit-op semantics, and value-edge cases (max ties, cmpu extensions, clb, rnd
tie-below, adc corners, -1.0^2 immediates).

Wedge-suspect / probe-first cases lead the bank and were run as
isolated boots (twice - the case bodies grew after the first probe
round) before the full capture. No wedges.

The capture caught THREE emulator defects, fixed before these goldens
were recorded (verdicts in docs/ARCHITECTURE-NOTES.md):

- **Bit-reverse addressing is uniformly rev-domain arithmetic**:
  r' = rev24(rev24(r) +- rev24(|N|)). The -Nn direction is a true
  reversed borrow (-N=8 from $001234 gives $001238, -N=3 gives
  $001236), not the +Nn walk; the plain +-1 toggle and N=0 no-op are
  emergent. Replaces the revbits/trailing-zeros model.
- **Multiply-with-shift counts >= 24 are a zero product** (like #0);
  the emulator's negative-shift arm computed S * 2^-n.
- **Modulo wrap correction is direction-gated**: an add corrects only
  past the high bound, a subtract only below the low bound - with the
  pointer outside a non-power-of-2 buffer, a decrement is plain
  arithmetic.

Also pinned (matching the emulator): SM does NOT clamp unsigned-mode
multiplies; RM+SM rounded-grid clamp; reserved scaling mode 3;
reserved M values behave as the emulator's fall-through (modulo /
freeze bands); SSL bit-ops rewrite in place with SP untouched; SP bit
ops are plain register rewrites; standard modulo with N > modulo
matches the pre-wrap model.

All 39 cases match the JIT in both --mode=step and --mode=block.
