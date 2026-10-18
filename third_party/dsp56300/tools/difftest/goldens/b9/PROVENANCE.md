# Bank b9 goldens provenance

Captured 2026-08-24 from real MCPX APU GP silicon (xbtest oracle, one fresh
boot per case). The SR-modes bank: first silicon coverage of SM saturation
mode (SR bit 20, ~25 emit_saturate_sm call sites previously unvalidated),
arithmetic and rounding under the S1:S0 scaling modes, Jcc on E under scaling,
and the cc disjunct matrix (GE/LT/GT/LE with V=1 manufactured via ori
#imm,ccr, NN/NR across the U/E arms) - a plain-N implementation of the
disjunct conditions would have passed every earlier bank.

The first SM case was run as an isolated single-case boot before the
bank capture. Every case restores SR to $C00300.

The capture caught two SM-mode emulator defects, fixed before these
goldens were recorded (silicon verdicts in docs/ARCHITECTURE-NOTES.md):

- **Rounding instructions clamp to the rounded grid** under SM: a
  saturating RND/MPYR/MACR leaves the low 24 bits zero
  ($00:7FFFFF:000000), unlike the plain-op rail $00:7FFFFF:FFFFFF.
- **CCR N/Z/U/E come from the post-saturation value** for the MAC
  family; the JIT previously computed E/U from the unclamped sum.

All 17 cases match the JIT in both --mode=step and --mode=block after
the fixes.
